/// Setup for experiments and provides helper methods for timing, testing psfs, and contexts
///
/// Created by Max Norfolk on 1/30/25.

#pragma once


#define ENABLE_TIMER
#include <random>
#include "plan/plan.h"
#include "lots_of_psfs.h"
using namespace fishstore::plan::data_frame;
using namespace fishstore::plan::data_frame::aggregate;
using namespace fishstore::core;

typedef fishstore::environment::QueueIoHandler handler_t;
typedef fishstore::device::FileSystemDisk<handler_t, 1073741824L> disk_t;
using adapter_t = fishstore::adapter::SIMDJsonAdapter;
// using disk_t = fishstore::device::NullDisk;


using score_t = double;
constexpr score_t MEM_FS_COST = 180.793798;
constexpr score_t DISK_FS_COST = 1630.423504;

constexpr score_t MEM_SCAN_COST_VERY_SEL_A = 49.849834;
constexpr score_t MEM_SCAN_COST_VERY_SEL_B = -64.087562;
constexpr score_t MEM_SCAN_COST_UNSEL_A = 207.423722;
constexpr score_t MEM_SCAN_COST_UNSEL_B = 3.203503;
constexpr score_t DISK_SCAN_COST_VERY_SEL_A = 180051.348537;
constexpr score_t DISK_SCAN_COST_VERY_SEL_B = 5573.145236;
constexpr score_t DISK_SCAN_COST_UNSEL_A = 5626.435472;
constexpr score_t DISK_SCAN_COST_UNSEL_B = 3041.368685;
constexpr score_t MEM_SKIP_COST_VERY_SEL_A = 562.544053;
constexpr score_t MEM_SKIP_COST_VERY_SEL_B = 38.217128;
constexpr score_t MEM_SKIP_COST_UNSEL_A = 192.601383;
constexpr score_t MEM_SKIP_COST_UNSEL_B = 3.608314;
constexpr score_t DISK_SKIP_COST_VERY_SEL_A = 184930.671383;
constexpr score_t DISK_SKIP_COST_VERY_SEL_B = 13548.954082;
constexpr score_t DISK_SKIP_COST_UNSEL_A = 1270.526991;
constexpr score_t DISK_SKIP_COST_UNSEL_B = 3076.619208;


inline size_t time(const std::string& prefix, const std::function<void()>& func) {
    TIMER_INIT();
    TIMER_START();
    func();
    TIMER_STOP();
    printf("[%s]: %ld nanoseconds (%.2f seconds)\n\n", prefix.c_str(), TIMER_RESULT(), TIMER_RESULT() * 1e-9);
    return TIMER_RESULT();;
}

inline size_t median(const std::string& prefix, const std::function<void()>&f, int trials = 3) {
    std::vector<size_t> timed_res;
    for (int i = 0; i < trials; ++i) {
        timed_res.emplace_back(time(prefix + std::string{" [Trial "} + std::to_string(i) + "]:", f));
    }
    std::sort(timed_res.begin(), timed_res.end());
    return timed_res.at(timed_res.size() / 2);
}

class JsonInlineScanContext : public fishstore::core::IAsyncContext {
public:
    JsonInlineScanContext(uint32_t psf_id, int32_t value)
        : psf_id_(psf_id), value_(value), cnt(0), res(nullptr) {}

    inline void Touch(const fishstore::core::Record* rec) {
        // printf("Record Hit: %.*s\n", payload_size, payload);
        ++cnt;
        *res = cnt;
    }

    inline void Finalize() {
        printf("%lu record has been touched...\n", cnt);
        if (res != nullptr)
            *res = cnt;
    }

    inline fishstore::core::KeyHash get_hash() const {
        return fishstore::core::KeyHash{fishstore::core::Utility::GetHashCode(psf_id_, value_)};
    }

    inline bool check(const fishstore::core::KeyPointer* kpt) {
        auto tmp = kpt->mode == 1 && kpt->inline_psf_id == psf_id_ && kpt->value == value_;

        return tmp;
    }

    uint64_t* res;
    uint64_t cnt;

protected:
    fishstore::core::Status DeepCopy_Internal(IAsyncContext*& context_copy) {
        return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

private:
    uint32_t psf_id_;
    int32_t value_;
};

class JsonFullScanContext : public fishstore::core::IAsyncContext {
public:
    JsonFullScanContext() = default;

    inline void Touch(const fishstore::core::Record* rec) {
        // printf("Record Hit: %.*s\n", payload_size, payload);
        ++cnt;
    }

    inline void Finalize() {
        printf("%lu record has been touched...\n", cnt);
        if (res != nullptr)
            *res = cnt;
    }

    inline bool check(const char* payload, uint32_t payload_size) {
        return true;
    }

    uint64_t cnt = 0;
    uint64_t* res = nullptr;

protected:
    fishstore::core::Status DeepCopy_Internal(fishstore::core::IAsyncContext* & context_copy) {
        return fishstore::core::IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }
};

template<typename A>
inline size_t testScan(std::string prefix, DataLoader<disk_t, A>& data, uint64_t s, uint64_t e,
                       uint64_t* cnt = nullptr) {
    auto callback = [](fishstore::core::IAsyncContext* ctxt, fishstore::core::Status result) {
        assert(result == fishstore::core::Status::Ok);
    };
    JsonFullScanContext ctx{};
    ctx.res = cnt;

    size_t nanos = time(std::string("Full Scan ") + prefix, [&] {
        data.fishstore->FullScan(ctx, callback, 1, s, e);
        data.fishstore->CompletePending(true);
    });


    return nanos;
}


inline size_t testPsf(std::string prefix, DataLoader<disk_t, adapter_t>& data, uint32_t id, uint64_t s, uint64_t e,
                      uint64_t* cnt = nullptr) {
    auto callback = [](fishstore::core::IAsyncContext* ctxt, fishstore::core::Status result) {
        assert(result == fishstore::core::Status::Ok);
    };
    JsonInlineScanContext ctx{id, 1};
    ctx.res = cnt;

    size_t nanos = time(std::string("Psf ") + std::to_string(id) + " " + prefix, [&] {
        data.fishstore->Scan(ctx, callback, 1, s, e);
        data.fishstore->CompletePending(true);
    });

    return nanos;
}
