import pandas as pd
import spacy
import time
from typing import List

# ----------- Load GitHub Issue Comments -----------

def load_issue_comments(filepath: str) -> List[str]:
    df = pd.read_json(filepath, lines=True)
    issue_comments = df[df['type'] == 'IssueCommentEvent'].copy()
    issue_comments['text'] = issue_comments['payload'].apply(
        lambda p: p.get('comment', {}).get('body', '') if isinstance(p, dict) else ''
    )
    issue_comments = issue_comments[issue_comments['text'].str.strip().astype(bool)]
    return issue_comments['text'].tolist()

# ----------- Transformation Functions -----------

def detect_rudeness(doc):
    rude_keywords = {'garbage', 'useless', 'stupid', 'broken', 'terrible', 'clearly', 'obviously'}
    rude = any(tok.text.lower() in rude_keywords for tok in doc)
    imperative = any(tok.tag_ == "VB" and tok.dep_ == "ROOT" and tok.text.istitle() for tok in doc)
    negation = any(tok.dep_ == "neg" for tok in doc)
    return rude or imperative or negation

def detect_bug_reproduction(doc):
    step_words = {'first', 'then', 'after', 'when', 'if'}
    has_steps = any(tok.text.lower() in step_words for tok in doc)
    environment_mention = any(ent.label_ in {'ORG', 'PRODUCT', 'GPE', 'LANGUAGE'} for ent in doc.ents)
    expected_vs_actual = any(tok.text.lower() in {'should', 'but'} for tok in doc)
    return has_steps or environment_mention or expected_vs_actual

def compute_throughput(start_time, end_time, total_bytes):
    duration = end_time - start_time
    gb = total_bytes / 1e9
    return gb / duration if duration > 0 else 0

# ----------- Benchmark Function -----------

def benchmark(filepath: str):
    nlp = spacy.load("en_core_web_sm")
    results = []

    # --------- Level 0: No optimization ---------
    # Call load twice, and nlp twice per comment
    start = time.time()
    comments1 = load_issue_comments(filepath)

    for comment in comments1:
        doc1 = nlp(comment)
        detect_rudeness(doc1)


    comments2 = load_issue_comments(filepath)

    for comment in comments2:
        doc2 = nlp(comment)
        detect_bug_reproduction(doc2)
    end = time.time()
    total_bytes = sum(len(c.encode('utf-8')) for c in comments1)

    results.append(("Level 0 - No Optimization", compute_throughput(start, end, total_bytes)))
    print("Level 0", end - start)

    # --------- Level 1: Reuse parsed comments, NLP per function ---------
    # Only load once, call nlp twice per comment

    start = time.time()
    comments = load_issue_comments(filepath)


    for comment in comments:
        doc1 = nlp(comment)
        detect_rudeness(doc1)
        doc2 = nlp(comment)
        detect_bug_reproduction(doc2)
    end = time.time()
    results.append(("Level 1 - Reuse Comments", compute_throughput(start, end, total_bytes)))
    print("Level 1", end - start)


    # --------- Level 2: Full reuse via nlp.pipe ---------
    # Load once, parse once
    start = time.time()
    for comment in comments:
        doc = nlp(comment)
        detect_rudeness(doc)
        detect_bug_reproduction(doc)
    end = time.time()
    results.append(("Level 2 - Reuse NLP Parsing", compute_throughput(start, end, total_bytes)))
    print("Level 2", end - start)


    return pd.DataFrame(results, columns=["Optimization Level", "Throughput (GB/s)"])

# ----------- Run Main -----------

if __name__ == "__main__":
    filepath = "/scratch/mnorfolk/Data/Github2025/2025-01-01-00.json"  # Replace with your GitHub Archive JSON file
    df = benchmark(filepath)
    print(df.to_string(index=False))
