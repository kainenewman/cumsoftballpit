import requests
from bs4 import Beautiful Soup
Example code (python)
URL =
fake-jobs/"
page = requests.get (URL)
"https://github.com/cumsoft
soup = Beautiful Soup (page.content,
"html.parser")
results =
soup.find(id="ResultsContainer")
job_elements
results.find_all("div",
class_="card-content")
python_jo = results.find_all(
"h2", string=lambda text:
"python" in text.lower()
)
python_job_elements [
=
h2_element.parent.parent.parent
for h2_element in python_jobs
]
=
for job_element in
python_job_elements:
|||
=
links
=
job_element.find_all("a")
for link in links:
link url =
print (f"Apply here:
{link_url}\n")
