import unicodedata, string

_ = string.ascii_letters

while True:
    inpt = unicodedata.normalize("NFKC", input("> "))
    
    for i in inpt:
        if i in _:
            raise NameError("No ASCII letters!")
    
    exec(inpt)
