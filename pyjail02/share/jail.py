import unicodedata

inpt = unicodedata.normalize("NFKC", input("> "))

print(eval(inpt, {"__builtins__":{}}, {}))
