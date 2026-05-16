import pdfplumber
import json

stations = []

with pdfplumber.open("listofstationstobetakenupbyirsdc147417.pdf") as pdf:
    for page in pdf.pages:

        tables = page.extract_tables()

        for table in tables:
            for row in table:

                # skip empty rows
                if not row or len(row) < 6:
                    continue

                # skip header row
                if row[0] == "SN":
                    continue

                try:
                    code = row[2]          # Station Code
                    name = row[3]          # Station Name
                    division = row[4]      # Division

                    # clean
                    if code and name and division:
                        stations.append({
                            "code": code.strip(),
                            "name": name.strip(),
                            "division": division.strip()
                        })

                except:
                    continue

# remove duplicates
unique = []
seen = set()

for s in stations:
    key = (s["code"], s["name"])
    if key not in seen:
        seen.add(key)
        unique.append(s)

# save file in backend folder
with open("../stations.json", "w") as f:
    json.dump(unique, f, indent=2)

print(f"✅ stations.json created! Total: {len(unique)} stations")