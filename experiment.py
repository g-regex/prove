#23/09/2020
#Author: Louise Beyers
# functions: identify takes:
#       -forml(formula) and 
#       -num(the user should enter 0 for now, this represents the level of unpacking that forml has gone through)
#
#stt is a list of the statements on the basic level
#ops is a list of the operators on the basic level
#next_lvl is a list of the indices of the statements that have brackets in them
#
#this can probably be done more elegantly and also it does not work as it should if you have multiple nested brackets in your initial formula

def identify(forml, num):
    lvl = 0
    stt = []
    ops = ''
    next_lvl = []
    
    for char in forml:
        if (lvl == 0):
            if (char == '['):
                lvl += 1
                stt.append('')
                ops = ops + '.'
            elif (char == ']'):
                print("Too many closing brackets. Detected at character " + i)
            else:
                ops = ops + char
        elif (lvl > 0):
            if (char == '['):
                lvl += 1
                if not(len(stt) in next_lvl):
                    next_lvl.append(len(stt))
            elif (char == ']'):
                lvl -= 1
            if (lvl>0):
                stt[-1] = stt[-1] + char
    print(next_lvl)
    unpacked = [stt, ops, num]
    if (len(next_lvl) == 0):
        return unpacked
    else:
        for i in next_lvl:
            print(stt[i-1])
            return[unpacked, identify(stt[i-1], num+1)]

