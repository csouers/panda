from panda import Panda
import time


def main():
  p = Panda()
  while 1:
    print('1')
    p.can_send(0x05, b"\x00\x00\x02\x00\x00\x00\x00\x00", 0)
    time.sleep(0.5)

    print('2')
    p.can_send(0x06, b"\x00\x00\x02\x00\x00\x00\x00\x00", 1)
    time.sleep(0.5)

    print('3')
    p.can_send(0x07, b"\x00\x00\x02\x00\x00\x00\x00\x00", 2)
    time.sleep(0.5)

    print('clearing')
    p.can_send(0x10, b"\x00\x00\x02\x00\x00\x00\x00\x00", 0)
    time.sleep(1)


if __name__ == '__main__':
  main()
