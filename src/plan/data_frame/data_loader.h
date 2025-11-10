// Used to setup a fishstore object, and load data
//
// Created by Max Norfolk on 1/16/24.
#pragma once

#include <regex>
#include <string>
#include <psu-util/timer.h>

#include "plan/types.h"
#include "core/fishstore.h"

#include "plan/data_frame/data_frame.h"

namespace fishstore::plan::data_frame {
    template<typename D, typename A>
    class DataLoader {
        template<typename D2, typename A2>
        friend
        class DataFrame;

    public:
        explicit DataLoader(size_t table_size = 8192, size_t log_size = 1LL << 33) {
            std::experimental::filesystem::create_directory("data_frame");
            fishstore = std::make_unique<core::FishStore<D, A>>(table_size, log_size, "data_frame");
            fishstore->StartSession();
        }

        void setTimeField(const std::string& field) {
            fishstore->SetTimeField(field);
        }

        PsfAddress getHeadAddress() { return fishstore->GetHeadAddress().control(); }

        PsfAddress getTailAddress() { return fishstore->GetTailAddress().control(); }

        std::unique_ptr<typename DataFrame<D, A>::TableScanHelper> Query(
            const std::vector<std::string>& fields, const std::vector<std::string>& origin = {}) {
            return std::make_unique<typename DataFrame<D, A>::TableScanHelper>(*this, fields, origin);
        }

        template<typename V>
        std::unique_ptr<typename DataFrame<D, A>::template PsfGroupByHelper<V>> PsfGroupBy(
            const std::string& ez_psf, const std::vector<std::string>& fields) {
            return std::make_unique<typename DataFrame<D, A>::template PsfGroupByHelper<V>>(*this, ez_psf, fields);
        }

        static std::vector<std::string> GenerateBatches(const std::string& path, size_t batch_size = 8,
                                                        bool append_line = false) {
            std::vector<std::string> ret;
            std::ifstream in{path};
            assert(in.good() &&"Failed to load file from path!");
            size_t count = 0;
            for (std::string line; std::getline(in, line);) {
                if (count == 0) {
                    ret.emplace_back("");
                }
                ret.back() += line;
                if (append_line)
                    ret.back() += "\n";
                ++count;
                if (count == batch_size)
                    count = 0;
            }

            in.close();
            return ret;
        }


        DataLoader& LoadFile(const std::string& path) {
            std::ifstream in{path};

            // https://stackoverflow.com/questions/12133379/c-using-ifstream-with-getline
            for (std::string line; std::getline(in, line);) {
                fishstore->BatchInsert(line, 1);
            }
            fishstore->Refresh();

            in.close();
            return *this;
        }

        DataLoader& LoadFileOneBatch(const std::string& path) {
            std::ifstream in{path, std::ios::binary | std::ios::ate};


            std::streamsize size = in.tellg();
            in.seekg(0, std::ios::beg);

            std::vector<char> buffer(size);
            if (in.read(buffer.data(), size)) {
                fishstore->BatchInsert(buffer.data(), 1);
            } else
                assert(0 && "Failed to load file from path!");


            in.close();
            return *this;
        }

        DataLoader& BatchInsert(const std::vector<std::string>& batches) {
            for (const auto& batch: batches) {
                fishstore->BatchInsert(batch, 1);
            }
            fishstore->Refresh();
            return *this;
        }

        template<typename It>
        DataLoader& BatchInsert(It start, It end) {
            for (It it = start; it != end; ++it)
                fishstore->BatchInsert(*it, 1);

            fishstore->Refresh();

            return *this;
        }

        DataLoader& Register(const std::string& ezpsf, const std::vector<std::string>& custom_naming = {}) {
            printf("\n\nStarting Register for [%s]!\n", ezpsf.c_str());
            // TIMER_INIT();
            // TIMER_START();
            auto psf_lookup = fishstore->MakeEzPsf(ezpsf, custom_naming);
            // TIMER_STOP();
            // printf("Creating the Function took %ld nanosecs [%f s]\n", TIMER_RESULT(), TIMER_RESULT() * 1e-9);
            // TIMER_START();
            core::ParserAction action{
                psf_lookup.type == core::PsfType::INLINE ? core::REGISTER_INLINE_PSF : core::REGISTER_GENERAL_PSF,
                psf_lookup.id
            };

            PsfAddress start_addr;
            std::vector<core::ParserAction> parser_actions;

            parser_actions.push_back(action);
            fishstore->ApplyParserShift(parser_actions, [&start_addr](PsfAddress s_addr) {
                start_addr = s_addr;
            });
            fishstore->CompleteAction(true);
            // TIMER_STOP();
            // printf("Adding PSF into system %ld nanosecs [%f s]\n", TIMER_RESULT(), TIMER_RESULT() * 1e-9);
            // TIMER_START();
            PsfInfo info{psf_lookup, start_addr, core::Address::kMaxAddress};
            psf_map.insert_or_assign(psf_lookup, info);
            named_psfs.insert_or_assign(ezpsf, psf_lookup);
            // TIMER_STOP();
            // printf("Inserting PSF into catalog %ld nanosecs [%f s]\n", TIMER_RESULT(), TIMER_RESULT() * 1e-9);

            printf("Inserting named Psf : [%s]\n", ezpsf.c_str());
            fflush(stdout);
            return *this;
        }

        DataLoader& Deregister(const std::string& ezpsf) {
            // TIMER_INIT();
            // TIMER_START();
            if (named_psfs.find(ezpsf) == named_psfs.end())
                return *this;

            core::PsfLookup psf_lookup = named_psfs.at(ezpsf);

            core::ParserAction action{
                psf_lookup.type == core::PsfType::INLINE ? core::DEREGISTER_INLINE_PSF : core::DEREGISTER_GENERAL_PSF,
                psf_lookup.id
            };

            std::vector<core::ParserAction> parser_actions;
            parser_actions.emplace_back(action);

            PsfAddress end_addr = fishstore->ApplyParserShift(
                parser_actions,
                [](PsfAddress s_addr) {});
            fishstore->CompleteAction(true);
            psf_map.at(psf_lookup).end = end_addr;
            // TIMER_STOP();
            // printf("Setting end boundary for PSF %ld nanosecs [%f s]\n", TIMER_RESULT(), TIMER_RESULT() * 1e-9);
            printf("End Address: %lu\n", end_addr);
            fflush(stdout);
            return *this;
        }

        // returns the id of a field, or FS if there is no valid id.
        [[nodiscard]] PsfId GetPsfId(const std::string& field) {
            const auto it = named_psfs.find(field);
            if (it == named_psfs.end()) {
                return FS_ID;
            }
            std::cout << "Found PSF for: [" << field << "]!" << std::endl;
            return it->second;
        }

        // gets the PsfInfo for a particular field
        [[nodiscard]] PsfInfo GetPsfInfo(const std::string& field) const {
            return psf_map.at(named_psfs.at(field));
        }

        [[nodiscard]] PsfInfo GetPsfInfo(PsfId id) const {
            return psf_map.at(id);
        }

        [[nodiscard]] fishstore::core::PsfLookup MakeEzPsf(std::string ez_psf) {
            for (const auto& macro: macros) {
                ez_psf = std::regex_replace(ez_psf, macro.first, macro.second);
            }
            return fishstore->MakeEzPsf(ez_psf);
        }

        void Rename(const std::string& old, const std::string& new_) { macros.emplace_back(old, new_); }

        void PrintRegistrations() {
            printf("Current Registrations:\n");
            for (const auto& named_psf: named_psfs) {
                PsfInfo info = psf_map.at(named_psf.second);
                if (info.end >= getHeadAddress()) {
                    // psf is not too old
                    if (info.end == core::Address::kMaxAddress)
                        printf("\t[PSF %s] from [%15lu,%15s]: '%s'\n",
                               info.id.toString().c_str(), info.start, "MaxAddress", named_psf.first.c_str());
                    else
                        printf("\t[PSF %s] from [%15lu, %15lu]: '%s'\n",
                               info.id.toString().c_str(), info.start, info.end,
                               named_psf.first.c_str());
                }
            }
            fflush(stdout);
        }

        void PrintPageTimestamps() {
            int i = 0;
            for (auto&& page_time: fishstore->page_times) {
                printf("Page %30d, %s\n", i++, page_time.c_str());
            }
        }

        void PrintPsfLens() {
            printf("PsfLens:\n");
            for (auto&& [hc, len]: fishstore->psf_len_ht) {
                printf("[%ld, %ld]\n", hc, len);
            }
        }

        void PrintOneHashes() {
            printf("PsfOneHashes:\n");
            for (auto && [name, lookup] : named_psfs) {
                printf("[%s, %ld]\n", name.c_str(), core::Utility::GetHashCode(lookup.id, 1));
            } 
        }

        std::unique_ptr<core::FishStore<D, A>> fishstore;
        PsfMap psf_map;

        std::unordered_map<std::string, core::PsfLookup> named_psfs;

        
    private:
        std::vector<std::pair<std::regex, std::string>> macros;
    };
}
