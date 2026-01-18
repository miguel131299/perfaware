#!/usr/bin/env python3
"""
Graph page fault test results from page_fault_tester.

Usage:
    # Run test and save output to file
    ./page_fault_tester 1000 > results_forward.csv
    ./page_fault_tester 1000 backward > results_backward.csv
    
    # Then graph it (single file)
    python3 graph_page_faults.py results_forward.csv
    
    # Or compare two files
    python3 graph_page_faults.py results_forward.csv results_backward.csv
    
    # Or pipe directly
    ./page_fault_tester 1000 | python3 graph_page_faults.py
"""

import sys
import csv
import matplotlib.pyplot as plt

def parse_csv(input_file):
    """Parse CSV from file or stdin."""
    touch_counts = []
    fault_counts = []
    extra_faults = []
    page_count = None
    
    # Read from file or stdin
    if input_file == '-' or input_file is None:
        lines = sys.stdin.readlines()
    else:
        with open(input_file, 'r') as f:
            lines = f.readlines()
    
    # Skip header lines until we find the CSV header
    csv_start = 0
    for i, line in enumerate(lines):
        if 'Page Count' in line and 'Touch Count' in line:
            csv_start = i + 1
            break
    
    # Parse CSV data
    reader = csv.DictReader(lines[csv_start:], fieldnames=['Page Count', 'Touch Count', 'Fault Count', 'Extra Faults'])
    
    for row in reader:
        if row['Page Count'] is None:
            continue
        try:
            page_count = int(row['Page Count'])
            touch_count = int(row['Touch Count'])
            fault_count = int(row['Fault Count'])
            extra_fault = int(row['Extra Faults'])
            
            touch_counts.append(touch_count)
            fault_counts.append(fault_count)
            extra_faults.append(extra_fault)
        except (ValueError, TypeError):
            continue
    
    return page_count, touch_counts, fault_counts, extra_faults

def plot_results(results_dict):
    """Create graphs of the page fault test results.
    
    results_dict: dictionary with label -> (page_count, touch_counts, fault_counts, extra_faults)
    """
    
    if len(results_dict) == 1:
        # Single file - use original layout
        label = list(results_dict.keys())[0]
        page_count, touch_counts, fault_counts, extra_faults = results_dict[label]
        
        fig, axes = plt.subplots(2, 1, figsize=(12, 8))
        fig.suptitle(f'Page Fault Analysis ({page_count} pages total)', fontsize=14, fontweight='bold')
        
        # Plot 1: Fault Count vs Touch Count
        ax1 = axes[0]
        ax1.plot(touch_counts, fault_counts, 'b-', marker='o', label='Actual Faults', linewidth=2, markersize=4)
        ax1.plot(touch_counts, touch_counts, 'r--', label='Expected (1:1)', linewidth=2, alpha=0.7)
        ax1.set_xlabel('Pages Touched', fontsize=11)
        ax1.set_ylabel('Page Faults', fontsize=11)
        ax1.set_title('Page Faults vs Pages Touched', fontsize=12)
        ax1.legend(fontsize=10)
        ax1.grid(True, alpha=0.3)
        
        # Plot 2: Extra Faults
        ax2 = axes[1]
        ax2.plot(touch_counts, extra_faults, 'g-', marker='s', linewidth=2, markersize=4)
        ax2.set_xlabel('Pages Touched', fontsize=11)
        ax2.set_ylabel('Extra Faults (overhead)', fontsize=11)
        ax2.set_title('Page Fault Overhead', fontsize=12)
        ax2.grid(True, alpha=0.3)
        ax2.axhline(y=0, color='r', linestyle='--', alpha=0.5)
        
    else:
        # Multiple files - show side-by-side comparison
        num_files = len(results_dict)
        
        fig, axes = plt.subplots(2, num_files, figsize=(8 * num_files, 10))
        if num_files == 1:
            axes = [[axes[0], axes[1]]]
        
        fig.suptitle('Page Fault Comparison', fontsize=14, fontweight='bold')
        
        for col_idx, (label, (page_count, touch_counts, fault_counts, extra_faults)) in enumerate(results_dict.items()):
            # Top plot: Fault Count vs Touch Count
            ax_top = axes[0, col_idx] if num_files > 1 else axes[0][0]
            ax_top.plot(touch_counts, fault_counts, 'b-', marker='o', label='Actual Faults', linewidth=2, markersize=5)
            ax_top.plot(touch_counts, touch_counts, 'r--', label='Expected (1:1)', linewidth=2, alpha=0.7)
            ax_top.set_xlabel('Pages Touched', fontsize=11)
            ax_top.set_ylabel('Page Faults', fontsize=11)
            ax_top.set_title(f'{label.capitalize()} - Faults vs Pages Touched', fontsize=12)
            ax_top.legend(fontsize=10)
            ax_top.grid(True, alpha=0.3)
            
            # Bottom plot: Extra Faults
            ax_bottom = axes[1, col_idx] if num_files > 1 else axes[1][0]
            ax_bottom.plot(touch_counts, extra_faults, 'g-', marker='s', linewidth=2, markersize=5)
            ax_bottom.set_xlabel('Pages Touched', fontsize=11)
            ax_bottom.set_ylabel('Extra Faults (overhead)', fontsize=11)
            ax_bottom.set_title(f'{label.capitalize()} - Overhead', fontsize=12)
            ax_bottom.grid(True, alpha=0.3)
            ax_bottom.axhline(y=0, color='r', linestyle='--', alpha=0.5)
    
    plt.tight_layout()
    
    # Save and show
    output_file = 'page_fault_results.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Graph saved to {output_file}")
    
    plt.show()

def main():
    if len(sys.argv) < 2:
        # Read from stdin
        input_files = [None]
        labels = ['stdin']
    else:
        input_files = sys.argv[1:]
        labels = [f.replace('results_', '').replace('.csv', '') for f in input_files]
    
    results_dict = {}
    
    for input_file, label in zip(input_files, labels):
        try:
            page_count, touch_counts, fault_counts, extra_faults = parse_csv(input_file)
            
            if not touch_counts:
                print(f"WARNING: No data found in {input_file}", file=sys.stderr)
                continue
            
            results_dict[label] = (page_count, touch_counts, fault_counts, extra_faults)
            
        except FileNotFoundError:
            print(f"ERROR: File not found: {input_file}", file=sys.stderr)
            sys.exit(1)
        except Exception as e:
            print(f"ERROR: {e}", file=sys.stderr)
            sys.exit(1)
    
    if not results_dict:
        print("ERROR: No data loaded", file=sys.stderr)
        sys.exit(1)
    
    plot_results(results_dict)

if __name__ == '__main__':
    main()

