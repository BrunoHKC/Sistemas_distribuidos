import os

for prob in [10,20,30,40,50,60]:
    for seed in range(0,50):
        os.system("./vcube 8 "+ str(prob)+" "+str(seed)+" > log.txt")
        content = open("log.txt","r").read()
        if content.find("ACONTECEU UM EVENTO INTERESSANTE NO TEMPO") != -1:
            print("log_"+str(prob)+"_"+str(seed))
            os.system("mv log.txt ./logs/log_"+str(prob)+"_"+str(seed))