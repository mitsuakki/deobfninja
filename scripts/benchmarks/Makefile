PYTHON = python3
PIP = pip
VENV_DIR = .venv
ACTIVATE = . $(VENV_DIR)/bin/activate

all: setup benchmark

setup:
	$(PYTHON) -m venv $(VENV_DIR)
	$(ACTIVATE) && $(PIP) install -r requirements.txt

benchmark:
	$(ACTIVATE) && $(PYTHON) script.py

clean:
	rm -rf $(VENV_DIR)

.PHONY: all setup benchmark clean