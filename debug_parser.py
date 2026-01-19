import re

with open('/home/radnus/Projects/fooyin/fooyin_tagger/annakili.html', 'r', encoding='utf-8') as f:
    html = f.read()

print('=== Debugging Wikipedia Parser ===\n')

# Test section extraction
print('1. Looking for Track Listing section:')
section_re = re.compile(r'<span[^>]*class="[^"]*mw-headline[^"]*"[^>]*id="(?:Soundtrack|Music|Track_listing)"[^>]*>.*?</span>', re.IGNORECASE | re.DOTALL)
section_match = section_re.search(html)

if section_match:
    print('✓ Section found!')
    print('  ID:', re.search(r'id="([^"]+)"', section_match.group()).group(1))
    print('  Content:', section_match.group())
    
    section_start = section_match.end()
    print(f'  Start index: {section_start}')
    
    # Find next section
    next_section_re = re.compile(r'<h[23][^>]*>', re.IGNORECASE)
    next_match = next_section_re.search(html, section_start)
    
    if next_match:
        print(f'  Next section found at: {next_match.start()}')
    else:
        print('  No next section found, using end of file')
        
    section_end = next_match.start() if next_match else len(html)
    
    print(f'  Extracting section from {section_start} to {section_end}')
    section_html = html[section_start:section_end]
    
    print(f'\n2. Section length: {len(section_html)} characters')
    print(f'3. First 200 characters of section:')
    print(repr(section_html[:200]))
    
    # Look for tracklist
    tracklist_re = re.compile(r'<table[^>]*class="[^"]*(?:wikitable|tracklist)[^"]*"[^>]*>(.*?)</table>', re.IGNORECASE | re.DOTALL)
    tracklist_match = tracklist_re.search(section_html)
    
    if tracklist_match:
        print('✓ Tracklist found!')
        tracklist_html = tracklist_match.group(1)
        
        print(f'\n4. Tracklist first 200 characters:')
        print(repr(tracklist_html[:200]))
        
        # Count tracks
        rows_re = re.compile(r'<tr[^>]*>(.*?)</tr>', re.IGNORECASE | re.DOTALL)
        rows = list(rows_re.finditer(tracklist_html))
        
        print(f'\n5. Tracklist rows found: {len(rows)}')
        
        if len(rows) > 1:
            print(f'\n6. First 3 data rows:')
            for i, row in enumerate(rows[1:4]):
                row_html = row.group(1)
                cells_re = re.compile(r'<t[hd][^>]*>(.*?)</t[hd]>', re.IGNORECASE | re.DOTALL)
                cells = list(cells_re.finditer(row_html))
                
                cell_texts = []
                for j, cell in enumerate(cells):
                    cell_html = cell.group(1)
                    # Strip tags
                    clean = re.sub(r'<[^>]+>', '', cell_html)
                    # Strip references
                    clean = re.sub(r'\[\d+\]', '', clean)
                    # Strip [edit]
                    clean = re.sub(r'\[edit\]', '', clean)
                    clean = clean.strip()
                    cell_texts.append(clean)
                    
                print(f'Row {i+1}: {cell_texts}')
        
    else:
        print('✗ No tracklist found')
        
        # Check all tables in section
        print('\nChecking all tables in section:')
        all_tables = re.findall(r'<table[^>]*>(.*?)</table>', section_html, re.DOTALL)
        print(f'Number of tables in section: {len(all_tables)}')
        
        if all_tables:
            print('First table:')
            first_table = all_tables[0]
            clean_table = re.sub(r'<[^>]+>', ' ', first_table)
            print(repr(clean_table[:200]))
            
else:
    print('✗ Section not found')
    
    # Look for any sections with possible variations
    print('\nLooking for all possible track-related sections:')
    all_sections = re.findall(r'<span[^>]*class="[^"]*mw-headline[^"]*"[^>]*id="([^"]+)"', html, re.IGNORECASE)
    
    for i, id_val in enumerate(all_sections):
        print(f'Section {i+1}: id="{id_val}"')
