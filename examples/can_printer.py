import time
import struct
from panda import Panda
import binascii
import sys

FAKED = True
TARGET_ADDRESSES = [0xAF87118, 0xAF87119]

class canList():
    def __init__(self):
        self.collector = []

    def append(self, cf):
        self.collector.append(cf)

    def print(self):
        if len(self.collector) == 0:
            return
        # dd = chr(27) + "[2J"
        # dd += "%5.2f\n" % (sec_since_boot() - start)
        # for k, v in sorted(zip(msgs.keys(), map(lambda x: binascii.hexlify(x[-1]), msgs.values()))):
        #     if max_msg is None or k < max_msg:
        #         dd += "%s(%6d) %s\n" % ("%04X(%4d)" % (k, k), len(msgs[k]), v.decode('ascii'))
        # print(dd)
        l = len(self.collector) - 1
        print(chr(27) + "[2J")
        for frame in range(0, len(self.collector)-2):
            print(self.collector[frame].addr, " - ", self.collector[frame].time, " - ", self.collector[frame].data, sep="   ")
        # print`` the last line
        print(self.collector[l].addr, " - ", self.collector[l].time, " - ", self.collector[l].data, sep="   ", end='\r')


class canFrame():
    def __init__(self, addr, ts, bus, data):
        self.addr = addr
        self.time = round(time.time(), 4)
        self.bus = bus
        self.data = data

    def __repr__(self):
        return str(self)

    def __str__(self):
        return str(self)

    def print(self):
        print(str((self.addr, self.time, self.data, self.bus)))

class FakePanda():
    def __init__(self):
        pass

def main(argv):
    if FAKED:
        pass
    else:
        p = Panda()
        p.set_safety_mode(100)


    can = canList()

    # init timers

    HZ_PRINT = 1
    HZ_READ = 1
    print_t = time.time()
    read_t = time.time()
    while (1):
        # print at 4hz
        ts = time.time()
        if ts - print_t >= HZ_PRINT:
            # reset timer
            base_t = ts

            can.print()
        if ts - read_t >= HZ_READ:
            read_t = ts

            if FAKED:
                msgs = [(0xAF87118, 1, b'', 1)]
                msgs.append((0xAF87119, 1, b'', 1))
            else:
                msgs = p.can_recv()
            if msgs is not None:
                for ids, ts, dat, bus in msgs:
                    if bus == 1 and ids in TARGET_ADDRESSES:
                        id = format(ids,'x')
                        data = binascii.hexlify(dat)
                        c = canFrame(id, ts, data, bus)
                        can.collector.append(c)
                        # print ("R:", bus, "-", binascii.hexlify(dat), "-", format(ids,'x'), time.time())
if __name__ == '__main__':
    main(sys.argv[1:])
