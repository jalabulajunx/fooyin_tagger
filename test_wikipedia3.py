import requests

headers = {
    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
}

url = "https://en.wikipedia.org/wiki/Annakili_(soundtrack)"
response = requests.get(url, headers=headers)

# Save to file for debugging
with open("annakili.html", "w", encoding="utf-8") as f:
    f.write(response.text)

print(f"Status code: {response.status_code}")
# Print first 1000 characters
print("\nFirst 1000 characters of HTML:")
print(response.text[:1000])
