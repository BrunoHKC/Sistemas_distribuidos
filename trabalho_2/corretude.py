import os

for prob in [50,70,80,90]:
    for seed in range(0,1000):
        os.system("./vcube 8 "+str(prob)+" "+str(seed)+" > log.txt")
        content = open("log.txt","r").read()
        print("/vcube 8 "+str(prob)+" "+str(seed))
        if content.find("Simulation Error") != -1:
            print("ERRO com "+str(prob)+" "+str(seed))
            exit()