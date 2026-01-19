import requests
from bs4 import BeautifulSoup

url = "https://en.wikipedia.org/wiki/Annakili_(soundtrack)"
response = requests.get(url)
soup = BeautifulSoup(response.text, 'html.parser')

# Print all section ids
print("All section ids on page:")
for tag in soup.find_all(['h1', 'h2', 'h3']):
    span = tag.find('span', class_='mw-headline')
    if span and 'id' in span.attrs:
        print(f"Section: {span['id']} - {span.text}")

print("\nAll tables on page:")
for table in soup.find_all('table'):
    print(f"Table with classes: {table.get('class')}")
    print(f"Table HTML snippet: {str(table)[:300]}...")
