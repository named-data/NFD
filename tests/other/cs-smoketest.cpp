/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "table/cs.hpp"
#include "table/cs-entry.hpp"
#include <ndn-cxx/security/key-chain.hpp>

namespace nfd {
namespace cs_smoketest {

static void
runStressTest()
{
  shared_ptr<Data> dataWorkload[70000];
  shared_ptr<Interest> interestWorkload[70000];

  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue,
                                        reinterpret_cast<const uint8_t*>(0), 0));

  // 182 MB in memory
  for (int i = 0; i < 70000; i++) {
    Name name("/stress/test");
    name.appendNumber(i % 4);
    name.appendNumber(i);

    shared_ptr<Interest> interest = make_shared<Interest>(name);
    interestWorkload[i] = interest;

    shared_ptr<Data> data = make_shared<Data>(name);
    data->setSignature(fakeSignature);
    dataWorkload[i] = data;
  }

  time::duration<double, boost::nano> previousResult(0);

  for (size_t nInsertions = 1000; nInsertions < 10000000; nInsertions *= 2) {
    Cs cs;
    srand(time::toUnixTimestamp(time::system_clock::now()).count());

    time::steady_clock::TimePoint startTime = time::steady_clock::now();

    size_t workloadCounter = 0;
    for (size_t i = 0; i < nInsertions; i++) {
      if (workloadCounter > 69999)
        workloadCounter = 0;

      cs.find(*interestWorkload[workloadCounter]);
      Data& data = *dataWorkload[workloadCounter];
      data.setName(data.getName()); // reset data.m_fullName
      data.wireEncode();
      cs.insert(data);

      workloadCounter++;
    }

    time::steady_clock::TimePoint endTime = time::steady_clock::now();

    time::duration<double, boost::nano> runDuration = endTime - startTime;
    time::duration<double, boost::nano> perOperationTime = runDuration / nInsertions;

    std::cout << "nItem = " << nInsertions << std::endl;
    std::cout << "Total running time = "
              << time::duration_cast<time::duration<double> >(runDuration)
              << std::endl;
    std::cout << "Average per-operation time = "
              << time::duration_cast<time::duration<double, boost::micro> >(perOperationTime)
              << std::endl;

    if (previousResult > time::nanoseconds(1))
      std::cout << "Change compared to the previous: "
                << (100.0 * perOperationTime / previousResult) << "%" << std::endl;

    std::cout << "\n=================================\n" << std::endl;

    previousResult = perOperationTime;
  }
}

} // namespace cs_smoketest
} // namespace nfd

int
main(int argc, char** argv)
{
  nfd::cs_smoketest::runStressTest();

  return 0;
}
