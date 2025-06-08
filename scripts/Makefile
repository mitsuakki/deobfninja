PYTHON = python3
PIP = pip
VENV_DIR = .venv
ACTIVATE = . $(VENV_DIR)/bin/activate

all: setup generate

setup:
	$(PYTHON) -m venv $(VENV_DIR)
	$(ACTIVATE) && $(PIP) install -r requirements.txt

generate:
	$(ACTIVATE) && $(PYTHON) generate_mba.py --numOfTerms 5 --numOfVars 2

clean:
	rm -rf $(VENV_DIR)

.PHONY: all setup generate clean