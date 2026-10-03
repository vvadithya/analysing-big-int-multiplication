import argparse
import os
import pandas as pd
from datetime import datetime
from reportlab.lib.pagesizes import letter
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Image, PageBreak, Table, TableStyle
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib import colors

def generate_report(data_dir, graphs_dir, output_file):
    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    doc = SimpleDocTemplate(output_file, pagesize=letter,
                            rightMargin=72, leftMargin=72,
                            topMargin=72, bottomMargin=36)
    
    styles = getSampleStyleSheet()
    styles.add(ParagraphStyle(name='Center', alignment=1))
    
    Story = []
    
    # 1. Title Page
    Story.append(Spacer(1, 2 * 72))
    Story.append(Paragraph("High-Precision Integer Multiplication Using String Chunking and Parallelism", styles['Title']))
    Story.append(Spacer(1, 0.5 * 72))
    Story.append(Paragraph("Empirical Analysis and Benchmarking Report", styles['Center']))
    Story.append(Spacer(1, 0.2 * 72))
    Story.append(Paragraph(f"Date: {datetime.now().strftime('%Y-%m-%d')}", styles['Center']))
    Story.append(PageBreak())
    
    # 2. Abstract
    Story.append(Paragraph("Abstract", styles['Heading1']))
    Story.append(Paragraph("This report presents a comprehensive benchmarking analysis of high-precision integer multiplication. We analyze conventional schoolbook multiplication, chunked string multiplication using variable chunk sizes (n=3 to n=10), parallelized chunked multiplication, and the Karatsuba divide-and-conquer algorithm. These algorithms are evaluated for performance, scalability, and efficiency, and compared against the production-grade boost::multiprecision::cpp_int library as a baseline.", styles['Normal']))
    Story.append(Spacer(1, 12))
    Story.append(Paragraph("The experiment aims to determine if grouping decimal digits into fixed-size chunks (treating them as base-10^n digits) reduces the overall operational overhead compared to single-digit schoolbook multiplication, and whether this method scales efficiently across multiple CPU threads.", styles['Normal']))
    
    Story.append(Paragraph("1. Introduction", styles['Heading1']))
    Story.append(Paragraph("Arbitrary-precision arithmetic is a fundamental problem in computer science. While standard machine words (64-bit or 128-bit) can natively multiply numbers up to ~38 decimal digits, cryptosystems and scientific computing frequently require multiplying numbers containing thousands or millions of digits.", styles['Normal']))
    Story.append(Spacer(1, 12))
    Story.append(Paragraph("This project explores string chunking—where a large decimal string is split into chunks of n digits. Each chunk fits in a native machine integer, allowing multiple decimal digits to be multiplied in a single hardware instruction. The project compares sequential chunking, parallel chunking (via thread pooling), recursive Karatsuba, and standard schoolbook multiplication.", styles['Normal']))
    
    Story.append(Paragraph("2. Mathematical Background", styles['Heading1']))
    Story.append(Paragraph("For a chunk size n, the input decimal strings are partitioned into blocks of n digits, essentially converting the number to base B = 10^n. The multiplication of two such numbers A and B can be expressed as polynomial multiplication evaluated at base B. Standard O(D^2) long multiplication applies, but the number of operations is reduced by a factor of n^2, at the cost of more complex individual operations and carry normalization.", styles['Normal']))
    
    Story.append(Paragraph("3. Algorithms & 4. Complexity", styles['Heading1']))
    Story.append(Paragraph("<b>Schoolbook:</b> O(D^2) multiplications, operating digit-by-digit.", styles['Normal']))
    Story.append(Paragraph("<b>Chunked:</b> Approximately O((D/n)^2) chunk-level multiplications. Uses 64-bit and 128-bit integers to prevent overflow during accumulation.", styles['Normal']))
    Story.append(Paragraph("<b>Parallel Chunked:</b> O(work/T + overhead). Divides the outer loop across T threads with private accumulation buffers to avoid locking, followed by a final reduction step.", styles['Normal']))
    Story.append(Paragraph("<b>Karatsuba:</b> O(D^1.585). A recursive divide-and-conquer approach that reduces the number of base multiplications from 4 to 3 per recursive step.", styles['Normal']))
    Story.append(PageBreak())

    Story.append(Paragraph("5. Experimental Setup", styles['Heading1']))
    sys_info_path = os.path.join(data_dir, "system_info.txt")
    if os.path.exists(sys_info_path):
        with open(sys_info_path, 'r') as f:
            for line in f:
                Story.append(Paragraph(line.strip(), styles['Normal']))
    else:
        Story.append(Paragraph("System info recorded automatically during benchmark execution.", styles['Normal']))
    
    # 6. Results & Graphs
    Story.append(Paragraph("6. Results & Scaling", styles['Heading1']))
    Story.append(Paragraph("The chart below illustrates the execution time scaling as the number of digits increases. Both axes use a logarithmic scale.", styles['Normal']))
    
    img_path1 = os.path.join(graphs_dir, "graph1_time_vs_digits.png")
    if os.path.exists(img_path1):
        Story.append(Spacer(1, 12))
        Story.append(Image(img_path1, width=450, height=300))
        Story.append(Spacer(1, 12))

    Story.append(Paragraph("7. Chunk Size Analysis", styles['Heading1']))
    Story.append(Paragraph("We analyzed chunk sizes from n=3 to n=10. As chunk size increases, the total number of required chunk multiplications decreases dramatically. However, the cost of individual operations (e.g., using 128-bit integers for n=10) and memory/cache effects creates a performance tradeoff.", styles['Normal']))
    
    img_path2 = os.path.join(graphs_dir, "graph2_time_vs_chunksize.png")
    if os.path.exists(img_path2):
        Story.append(Spacer(1, 12))
        Story.append(Image(img_path2, width=450, height=300))
        Story.append(Spacer(1, 12))
    
    img_path3 = os.path.join(graphs_dir, "graph3_chunkmuls_vs_chunksize.png")
    if os.path.exists(img_path3):
        Story.append(Spacer(1, 12))
        Story.append(Image(img_path3, width=450, height=300))
        Story.append(PageBreak())

    Story.append(Paragraph("8. Parallelism Analysis", styles['Heading1']))
    Story.append(Paragraph("By utilizing thread pools and range-partitioning, the chunked algorithm can be parallelized. The graphs below demonstrate the execution time scaling with thread count and the resulting parallel efficiency.", styles['Normal']))
    
    img_path5 = os.path.join(graphs_dir, "graph5_parallel_time_vs_threads.png")
    if os.path.exists(img_path5):
        Story.append(Spacer(1, 12))
        Story.append(Image(img_path5, width=450, height=300))
    
    img_path7 = os.path.join(graphs_dir, "graph7_parallel_efficiency.png")
    if os.path.exists(img_path7):
        Story.append(Spacer(1, 12))
        Story.append(Image(img_path7, width=450, height=300))
        Story.append(PageBreak())

    Story.append(Paragraph("9. Karatsuba & 10. Boost Comparison", styles['Heading1']))
    Story.append(Paragraph("For very large numbers, the asymptotic superiority of Karatsuba (O(D^1.585)) overtakes the O(D^2) chunked approach. Boost's cpp_int represents a highly optimized, production-ready baseline for arbitrary-precision arithmetic.", styles['Normal']))
    
    img_path9 = os.path.join(graphs_dir, "graph9_chunked_vs_karatsuba_vs_boost.png")
    if os.path.exists(img_path9):
        Story.append(Spacer(1, 12))
        Story.append(Image(img_path9, width=450, height=300))
        Story.append(Spacer(1, 12))

    Story.append(Paragraph("11. Discussion & Conclusion", styles['Heading1']))
    Story.append(Paragraph("String chunking provides a massive constant-factor speedup over traditional single-digit schoolbook multiplication by maximizing the utilization of native 64/128-bit machine instructions. However, it retains a quadratic time complexity. Parallelization provides near-linear speedup for sufficiently large inputs but suffers from thread overhead and reduction costs for small inputs. Ultimately, while optimized parallel chunking is highly performant, sub-quadratic algorithms like Karatsuba are strictly necessary for multiplying extremely large integers.", styles['Normal']))
    
    doc.build(Story)
    print(f"Generated report at {output_file}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--data-dir', default='results/')
    parser.add_argument('--graphs-dir', default='results/graphs/')
    parser.add_argument('--output', default='report/analysis.pdf')
    args = parser.parse_args()
    
    generate_report(args.data_dir, args.graphs_dir, args.output)
