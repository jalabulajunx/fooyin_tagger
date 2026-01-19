import requests
from bs4 import BeautifulSoup

url = "https://en.wikipedia.org/wiki/Annakili_(soundtrack)#Track_listing"
response = requests.get(url)
soup = BeautifulSoup(response.text, 'html.parser')

# Find section with id="Track_listing"
track_section = soup.find(id="Track_listing")
if track_section:
    print("Found Track_listing section")
    # Get parent h2 or h3
    parent = track_section.find_parent(['h2', 'h3'])
    if parent:
        print("Parent header:", parent.name)
        # Find next sibling that is a table
        next_sibling = parent.find_next_sibling()
        while next_sibling:
            if next_sibling.name == 'table':
                print("Found table:", next_sibling.get('class'))
                break
            next_sibling = next_sibling.find_next_sibling()
        if not next_sibling:
            print("No table found after Track_listing section")
    else:
        print("No parent header found for Track_listing")
else:
    print("No Track_listing section found")

# Check all tables on the page
print("\nAll tables on page:")
for table in soup.find_all('table'):
    print(f"Table with classes: {table.get('class')}")
