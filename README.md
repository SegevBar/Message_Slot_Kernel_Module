# OS HW 3 Tester
## Setup
1. In your project's directory, run the following command to download the tester script:
```sh
curl -o test_integration.py https://gist.githubusercontent.com/natexcvi/ceb4d019a976bd16761905c55dcc4ea3/raw/53f0fcc4537dd4f8cdb2278581a06ff0f7859fea/test_integration.py
```
2. Install Python3 and pip if needed using the command:
```sh
sudo apt update && sudo apt install python3 python3-pip
```
3. Install pytest:
```sh
pip3 install pytest
```
4. Make sure to change the `MAJOR` constant at the top of the `test_integration.py` file to match your code.
## Running the Test
Use the command:
```sh
python3 -m pytest --verbosity=1
```
> If you get an error about `pytest` not being found, run the command: `export PATH=$PATH:$HOME/.local/bin`.