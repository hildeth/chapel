module RARandomStream {
  param randWidth = 64;
  type randType = uint(randWidth);

  const bitDom = [0..#randWidth],
        m2: [bitDom] randType = computeM2Vals(randWidth);

  //
  // TODO: Remove when no longer necessary
  //
  def RAStream() {
    halt("sequential iterator called");
    yield getNthRandom(0:int(64));
  }

  //
  // TODO: Remove when no longer necessary
  //
  def RAStream(param tag: iterator) where tag == iterator.leader {
    halt("leader called");
    yield [0..10:uint(64)];
  }

  def RAStream(param tag: iterator, follower) where tag == iterator.follower {
    var val = getNthRandom(follower.low);
    for follower {
      getNextRandom(val);
      yield val;
    }
  }


  def getNthRandom(in n) {
    param period = 0x7fffffffffffffff/7;

    n %= period;
    if (n == 0) then return 0x1;
    var ran: randType = 0x2;
    for i in 0..log2(n)-1 by -1 {
      var val: randType = 0;
      for j in bitDom do
        if ((ran >> j) & 1) then val ^= m2(j);
      ran = val;
      if ((n >> i) & 1) then getNextRandom(ran);
    }
    return ran;
  }


  def getNextRandom(inout x) {
    param POLY = 0x7;
    param hiRandBit = 0x1:randType << (randWidth-1);

    x = (x << 1) ^ (if (x & hiRandBit) then POLY else 0);
  }


  def computeM2Vals(numVals) {
    var nextVal = 0x1: randType;
    for i in 1..numVals {
      yield nextVal;
      getNextRandom(nextVal);
      getNextRandom(nextVal);
    }
  }
}

