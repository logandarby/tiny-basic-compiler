#!/usr/bin/env python3
"""
Script to duplicate BASIC code with unique variable and label names
Usage: python duplicate_basic.py input.basic output.basic [copies]
"""

import sys
import re

def extract_variables_and_labels(content):
    """Extract all variable names and labels from BASIC code"""
    variables = set()
    labels = set()
    
    lines = content.split('\n')
    for line in lines:
        line = line.strip()
        
        # Extract labels (LABEL keyword)
        label_match = re.match(r'^LABEL\s+(\w+)', line)
        if label_match:
            labels.add(label_match.group(1))
        
        # Extract variables from LET statements
        let_match = re.match(r'^LET\s+(\w+)\s*=', line)
        if let_match:
            variables.add(let_match.group(1))
        
        # Extract variables from INPUT statements
        input_match = re.search(r'INPUT\s+(\w+)', line)
        if input_match:
            variables.add(input_match.group(1))
        
        # Extract variables from GOTO statements (labels)
        goto_match = re.search(r'GOTO\s+(\w+)', line)
        if goto_match:
            labels.add(goto_match.group(1))
        
        # Extract variables from IF conditions and expressions
        # This is a simple approach - matches word boundaries
        if_matches = re.findall(r'\b([a-zA-Z_]\w*)\b', line)
        for match in if_matches:
            # Skip BASIC keywords
            keywords = {'PRINT', 'LET', 'INPUT', 'IF', 'THEN', 'ENDIF', 'GOTO', 'LABEL'}
            if match not in keywords and not match.isdigit():
                # Could be a variable, but we'll be conservative
                # and only add if it appears in certain contexts
                if ('=' in line and match in line) or ('INPUT' in line and match in line):
                    variables.add(match)
    
    return variables, labels

def replace_identifiers(content, variables, labels, suffix):
    """Replace all variables and labels with suffixed versions"""
    modified_content = content
    
    # Replace labels in LABEL statements
    for label in labels:
        # Replace LABEL definitions
        modified_content = re.sub(
            r'\bLABEL\s+' + re.escape(label) + r'\b',
            f'LABEL {label}_{suffix}',
            modified_content
        )
        # Replace GOTO references
        modified_content = re.sub(
            r'\bGOTO\s+' + re.escape(label) + r'\b',
            f'GOTO {label}_{suffix}',
            modified_content
        )
    
    # Replace variables
    for var in variables:
        # Use word boundaries to avoid partial matches
        modified_content = re.sub(
            r'\b' + re.escape(var) + r'\b',
            f'{var}_{suffix}',
            modified_content
        )
    
    return modified_content

def duplicate_basic_file(input_file, output_file, num_copies=10):
    """Main function to duplicate BASIC file with unique identifiers"""
    
    try:
        with open(input_file, 'r') as f:
            original_content = f.read()
    except FileNotFoundError:
        print(f"Error: Input file '{input_file}' not found")
        return False
    
    # Extract variables and labels from original
    variables, labels = extract_variables_and_labels(original_content)
    
    print(f"Found {len(variables)} variables: {', '.join(sorted(variables))}")
    print(f"Found {len(labels)} labels: {', '.join(sorted(labels))}")
    print(f"Creating {num_copies} copies...")
    
    # Generate the duplicated content
    all_content = []
    
    for i in range(num_copies):
        suffix = f"copy{i}"
        if i == 0:
            # Keep the first copy unchanged
            all_content.append(original_content)
        else:
            # Modify subsequent copies
            modified = replace_identifiers(original_content, variables, labels, suffix)
            all_content.append(modified)
        
        # Add separator between copies
        if i < num_copies - 1:
            all_content.append(f"\nREM ===== COPY {i+1} ENDS, COPY {i+2} BEGINS =====\n")
    
    # Write output
    try:
        with open(output_file, 'w') as f:
            f.write('\n'.join(all_content))
        
        print(f"Successfully created '{output_file}' with {num_copies} copies")
        
        # Show file statistics
        total_lines = sum(content.count('\n') + 1 for content in all_content)
        print(f"Total lines: {total_lines}")
        
        with open(output_file, 'r') as f:
            file_size = len(f.read())
        print(f"File size: {file_size:,} bytes")
        
        return True
        
    except Exception as e:
        print(f"Error writing output file: {e}")
        return False

def main():
    if len(sys.argv) < 3:
        print("Usage: python duplicate_basic.py input.basic output.basic [copies]")
        print("Example: python duplicate_basic.py bank_account.basic large_test.basic 50")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    num_copies = int(sys.argv[3]) if len(sys.argv) > 3 else 10
    
    if num_copies < 1:
        print("Number of copies must be at least 1")
        sys.exit(1)
    
    success = duplicate_basic_file(input_file, output_file, num_copies)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
