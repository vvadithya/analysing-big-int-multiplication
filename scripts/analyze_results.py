import argparse
import os
import pandas as pd
import numpy as np

def analyze_results(input_file, output_dir):
    print(f"Reading raw data from {input_file}...")
    try:
        df = pd.read_csv(input_file)
    except FileNotFoundError:
        print(f"Error: Could not find {input_file}")
        return

    os.makedirs(output_dir, exist_ok=True)

    # Convert numeric columns
    numeric_cols = ['time_ns', 'multiplications', 'additions', 'carry_operations', 'subtractions', 
                    'recursive_calls', 'base_case_multiplications', 'normalizations', 
                    'reduction_operations', 'local_accumulations', 'num_chunks_a', 'num_chunks_b', 
                    'chunk_multiplications', 'max_recursion_depth', 'thread_count', 'work_per_thread']
    for col in numeric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    # Compute summary statistics
    group_cols = ['algorithm', 'chunk_size', 'digits', 'threads']
    
    summary = df.groupby(group_cols)['time_ns'].agg(
        mean_ns='mean',
        median_ns='median',
        min_ns='min',
        max_ns='max',
        stddev_ns='std'
    ).reset_index()

    summary['cv'] = summary['stddev_ns'] / summary['mean_ns']

    # Compute speedup vs schoolbook
    schoolbook = summary[summary['algorithm'] == 'schoolbook'][['digits', 'mean_ns']].rename(columns={'mean_ns': 'sb_mean_ns'})
    if not schoolbook.empty:
        summary = summary.merge(schoolbook, on='digits', how='left')
        summary['speedup_vs_schoolbook'] = summary['sb_mean_ns'] / summary['mean_ns']
        summary.drop(columns=['sb_mean_ns'], inplace=True)
    else:
        summary['speedup_vs_schoolbook'] = np.nan

    # Compute parallel speedup & efficiency
    seq_chunked = summary[(summary['algorithm'] == 'parallel_chunked') & (summary['threads'] == 1)][['chunk_size', 'digits', 'mean_ns']].rename(columns={'mean_ns': 'seq_mean_ns'})
    if not seq_chunked.empty:
        summary = summary.merge(seq_chunked, on=['chunk_size', 'digits'], how='left')
        
        def calc_parallel_speedup(row):
            if row['algorithm'] == 'parallel_chunked' and pd.notna(row['seq_mean_ns']):
                return row['seq_mean_ns'] / row['mean_ns']
            return np.nan
            
        summary['parallel_speedup'] = summary.apply(calc_parallel_speedup, axis=1)
        summary['parallel_efficiency'] = summary['parallel_speedup'] / summary['threads']
        summary.drop(columns=['seq_mean_ns'], inplace=True)
    else:
        summary['parallel_speedup'] = np.nan
        summary['parallel_efficiency'] = np.nan

    # Save summary statistics
    summary_file = os.path.join(output_dir, 'summary_statistics.csv')
    summary.to_csv(summary_file, index=False)
    print(f"Saved summary statistics to {summary_file}")

    # Compute operation counts
    op_cols = ['multiplications', 'additions', 'carry_operations', 'subtractions', 
               'recursive_calls', 'base_case_multiplications', 'normalizations', 
               'reduction_operations', 'local_accumulations', 'num_chunks_a', 'num_chunks_b', 
               'chunk_multiplications', 'max_recursion_depth']
    
    op_cols_exist = [c for c in op_cols if c in df.columns]
    
    if op_cols_exist:
        op_counts = df.groupby(group_cols)[op_cols_exist].mean().reset_index()
        op_counts_file = os.path.join(output_dir, 'operation_counts.csv')
        op_counts.to_csv(op_counts_file, index=False)
        print(f"Saved operation counts to {op_counts_file}")
    
    print("\nSummary Table:")
    print(summary.head(20).to_string())

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Analyze benchmarking results.')
    parser.add_argument('--input', default='results/raw/benchmark_results.csv', help='Path to raw benchmark results CSV')
    parser.add_argument('--output-dir', default='results/processed/', help='Directory to save processed results')
    args = parser.parse_args()
    
    analyze_results(args.input, args.output_dir)
