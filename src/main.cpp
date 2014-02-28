/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "nrd.hpp"

int
main(int argc, char** argv)
{
  try {
    ndn::nrd::Nrd nrd;
    nrd.enableLocalControlHeader();
    nrd.listen();
  }
  catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
