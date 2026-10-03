import argparse
import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

def generate_graphs(summary_file, raw_file, output_dir):
    try:
        summary = pd.read_csv(summary_file)
    except FileNotFoundError:
        print(f"Error: {summary_file} not found. Please run analyze_results.py first.")
        return

    try:
        raw_df = pd.read_csv(raw_file)
    except FileNotFoundError:
        raw_df = pd.DataFrame()

    os.makedirs(output_dir, exist_ok=True)
    plt.style.use('seaborn-v0_8-whitegrid')

    def save_plot(name):
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, name), dpi=300)
        plt.close()

    # 1. time vs digits (log-log)
    plt.figure(figsize=(10, 7))
    for algo in summary['algorithm'].unique():
        if algo == 'chunked':
            for cs in summary[summary['algorithm'] == algo]['chunk_size'].unique():
                subset = summary[(summary['algorithm'] == algo) & (summary['chunk_size'] == cs)]
                plt.plot(subset['digits'], subset['mean_ns'], marker='o', label=f'chunked n={cs}')
        elif algo == 'parallel_chunked':
            continue # skip in general plot, or pick max threads
        else:
            subset = summary[summary['algorithm'] == algo]
            plt.plot(subset['digits'], subset['mean_ns'], marker='s', label=algo)
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Number of Digits')
    plt.ylabel('Execution Time (ns)')
    plt.title('Execution Time vs. Number of Digits')
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.grid(True, which="both", ls="--")
    save_plot('graph1_time_vs_digits.png')

    # 2. time vs chunksize
    plt.figure(figsize=(10, 7))
    chunked_data = summary[summary['algorithm'] == 'chunked']
    if not chunked_data.empty:
        for digits in chunked_data['digits'].unique():
            subset = chunked_data[chunked_data['digits'] == digits].sort_values('chunk_size')
            plt.plot(subset['chunk_size'], subset['mean_ns'], marker='o', label=f'{digits} digits')
    plt.xlabel('Chunk Size')
    plt.ylabel('Execution Time (ns)')
    plt.title('Execution Time vs. Chunk Size')
    plt.legend()
    plt.grid(True)
    save_plot('graph2_time_vs_chunksize.png')

    # 3. chunkmuls vs chunksize
    plt.figure(figsize=(10, 7))
    if not raw_df.empty and 'chunk_multiplications' in raw_df.columns:
        chunk_raw = raw_df[raw_df['algorithm'] == 'chunked'].groupby(['digits', 'chunk_size'])['chunk_multiplications'].mean().reset_index()
        for digits in chunk_raw['digits'].unique():
            subset = chunk_raw[chunk_raw['digits'] == digits].sort_values('chunk_size')
            plt.plot(subset['chunk_size'], subset['chunk_multiplications'], marker='o', label=f'{digits} digits')
    plt.xlabel('Chunk Size')
    plt.ylabel('Number of Chunk Multiplications')
    plt.title('Chunk Multiplications vs. Chunk Size')
    plt.legend()
    plt.grid(True)
    save_plot('graph3_chunkmuls_vs_chunksize.png')
    
    # 4. chunksize vs time (bar chart)
    plt.figure(figsize=(10, 7))
    if not chunked_data.empty:
        largest_digits = chunked_data['digits'].max()
        subset = chunked_data[chunked_data['digits'] == largest_digits].sort_values('chunk_size')
        plt.bar(subset['chunk_size'].astype(str), subset['mean_ns'])
    plt.xlabel('Chunk Size')
    plt.ylabel('Execution Time (ns)')
    plt.title(f'Chunk Size vs. Time (Bar Chart) for {largest_digits if not chunked_data.empty else "N/A"} digits')
    plt.grid(axis='y')
    save_plot('graph4_chunksize_vs_time.png')

    # 5. parallel time vs threads
    plt.figure(figsize=(10, 7))
    par_data = summary[summary['algorithm'] == 'parallel_chunked']
    if not par_data.empty:
        largest_digits = par_data['digits'].max()
        par_subset = par_data[par_data['digits'] == largest_digits]
        for cs in par_subset['chunk_size'].unique():
            sub = par_subset[par_subset['chunk_size'] == cs].sort_values('threads')
            plt.plot(sub['threads'], sub['mean_ns'], marker='o', label=f'n={cs}')
    plt.xlabel('Thread Count')
    plt.ylabel('Execution Time (ns)')
    plt.title('Parallel Execution Time vs Thread Count')
    plt.legend()
    plt.grid(True)
    save_plot('graph5_parallel_time_vs_threads.png')

    # 6. parallel speedup
    plt.figure(figsize=(10, 7))
    if not par_data.empty:
        largest_digits = par_data['digits'].max()
        par_subset = par_data[par_data['digits'] == largest_digits]
        max_threads = par_subset['threads'].max()
        plt.plot([1, max_threads], [1, max_threads], 'k--', label='Ideal Speedup')
        for cs in par_subset['chunk_size'].unique():
            sub = par_subset[par_subset['chunk_size'] == cs].sort_values('threads')
            plt.plot(sub['threads'], sub['parallel_speedup'], marker='o', label=f'n={cs}')
    plt.xlabel('Thread Count')
    plt.ylabel('Speedup')
    plt.title('Parallel Speedup vs Thread Count')
    plt.legend()
    plt.grid(True)
    save_plot('graph6_parallel_speedup.png')

    # 7. parallel efficiency
    plt.figure(figsize=(10, 7))
    if not par_data.empty:
        largest_digits = par_data['digits'].max()
        par_subset = par_data[par_data['digits'] == largest_digits]
        max_threads = par_subset['threads'].max()
        plt.plot([1, max_threads], [1, 1], 'k--', label='100% Efficiency')
        for cs in par_subset['chunk_size'].unique():
            sub = par_subset[par_subset['chunk_size'] == cs].sort_values('threads')
            plt.plot(sub['threads'], sub['parallel_efficiency'], marker='o', label=f'n={cs}')
    plt.xlabel('Thread Count')
    plt.ylabel('Efficiency')
    plt.title('Parallel Efficiency vs Thread Count')
    plt.legend()
    plt.grid(True)
    save_plot('graph7_parallel_efficiency.png')

    # 8. chunked vs schoolbook
    plt.figure(figsize=(10, 7))
    if not chunked_data.empty and 'speedup_vs_schoolbook' in chunked_data.columns:
        for cs in chunked_data['chunk_size'].unique():
            sub = chunked_data[chunked_data['chunk_size'] == cs].sort_values('digits')
            plt.plot(sub['digits'], sub['speedup_vs_schoolbook'], marker='o', label=f'n={cs}')
    plt.xscale('log')
    plt.xlabel('Number of Digits')
    plt.ylabel('Speedup Ratio (Chunked / Schoolbook)')
    plt.title('Chunked vs Schoolbook Speedup')
    plt.legend()
    plt.grid(True)
    save_plot('graph8_chunked_vs_schoolbook.png')

    # 9. chunked vs karatsuba vs boost
    plt.figure(figsize=(10, 7))
    algos_comp = ['chunked', 'karatsuba', 'boost_cpp_int']
    comp_data = summary[summary['algorithm'].isin(algos_comp)]
    for algo in algos_comp:
        if algo == 'chunked':
            # take best chunked
            best_chunked = comp_data[comp_data['algorithm'] == 'chunked'].loc[
                comp_data[comp_data['algorithm'] == 'chunked'].groupby('digits')['mean_ns'].idxmin()
            ].sort_values('digits')
            plt.plot(best_chunked['digits'], best_chunked['mean_ns'], marker='o', label='best chunked')
        else:
            sub = comp_data[comp_data['algorithm'] == algo].sort_values('digits')
            plt.plot(sub['digits'], sub['mean_ns'], marker='s', label=algo)
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Number of Digits')
    plt.ylabel('Execution Time (ns)')
    plt.title('Best Chunked vs Karatsuba vs Boost')
    plt.legend()
    plt.grid(True)
    save_plot('graph9_chunked_vs_karatsuba_vs_boost.png')

    # 10. opcount vs digits
    plt.figure(figsize=(10, 7))
    if not raw_df.empty and 'multiplications' in raw_df.columns:
        op_df = raw_df.groupby(['algorithm', 'digits'])['multiplications'].mean().reset_index()
        for algo in op_df['algorithm'].unique():
            sub = op_df[op_df['algorithm'] == algo].sort_values('digits')
            plt.plot(sub['digits'], sub['multiplications'], marker='o', label=algo)
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Number of Digits')
    plt.ylabel('Multiplications')
    plt.title('Operation Count vs Digits')
    plt.legend()
    plt.grid(True)
    save_plot('graph10_opcount_vs_digits.png')

    # 11. heatmap chunksize vs threads
    plt.figure(figsize=(10, 7))
    if not par_data.empty:
        largest_digits = par_data['digits'].max()
        par_subset = par_data[par_data['digits'] == largest_digits]
        heatmap_data = par_subset.pivot(index='chunk_size', columns='threads', values='mean_ns')
        sns.heatmap(heatmap_data, annot=True, fmt=".2e", cmap="YlGnBu")
        plt.title(f'Heatmap of Time (ns) by Chunk Size and Threads (Digits={largest_digits})')
    save_plot('graph11_heatmap_chunksize_threads.png')

    # 12. scaling comparison
    plt.figure(figsize=(10, 7))
    for algo in summary['algorithm'].unique():
        sub = summary[summary['algorithm'] == algo].groupby('digits')['mean_ns'].mean().reset_index()
        plt.plot(sub['digits'], sub['mean_ns'], marker='o', label=algo)
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Number of Digits')
    plt.ylabel('Mean Execution Time (ns)')
    plt.title('Scaling Comparison of All Algorithms')
    plt.legend()
    plt.grid(True)
    save_plot('graph12_scaling_comparison.png')
    
    print(f"Generated all graphs in {output_dir}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', default='results/processed/summary_statistics.csv')
    parser.add_argument('--raw', default='results/raw/benchmark_results.csv')
    parser.add_argument('--output-dir', default='results/graphs/')
    args = parser.parse_args()
    
    generate_graphs(args.input, args.raw, args.output_dir)
