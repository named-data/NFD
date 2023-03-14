/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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

#include "mgmt/command-authenticator.hpp"

#include "manager-common-fixture.hpp"

namespace nfd::tests {

class CommandAuthenticatorFixture : public InterestSignerFixture
{
protected:
  void
  makeModules(std::initializer_list<std::string> modules)
  {
    for (const auto& module : modules) {
      authorizations.emplace(module, authenticator->makeAuthorization(module, "verb"));
    }
  }

  void
  loadConfig(const std::string& config)
  {
    ConfigFile cf;
    authenticator->setConfigFile(cf);
    cf.parse(config, false, "command-authenticator-test.conf");
  }

  bool
  authorize(const std::string& module, const Name& identity,
            const std::function<void(Interest&)>& modifyInterest = nullptr,
            ndn::security::SignedInterestFormat format = ndn::security::SignedInterestFormat::V03)
  {
    Interest interest = makeControlCommandRequest(Name("/prefix/" + module + "/verb"),
                                                  {}, format, identity);
    if (modifyInterest) {
      modifyInterest(interest);
    }

    const auto& authorization = authorizations.at(module);

    bool isAccepted = false;
    bool isRejected = false;
    authorization(Name("/prefix"), interest, nullptr,
      [this, &isAccepted, &isRejected] (const std::string& requester) {
        BOOST_REQUIRE_MESSAGE(!isAccepted && !isRejected,
                              "authorization function should invoke only one continuation");
        isAccepted = true;
        lastRequester = requester;
      },
      [this, &isAccepted, &isRejected] (ndn::mgmt::RejectReply act) {
        BOOST_REQUIRE_MESSAGE(!isAccepted && !isRejected,
                              "authorization function should invoke only one continuation");
        isRejected = true;
        lastRejectReply = act;
      });

    this->advanceClocks(1_ms, 10);
    BOOST_REQUIRE_MESSAGE(isAccepted || isRejected,
                          "authorization function should invoke one continuation");
    return isAccepted;
  }

protected:
  shared_ptr<CommandAuthenticator> authenticator = CommandAuthenticator::create();
  std::unordered_map<std::string, ndn::mgmt::Authorization> authorizations;
  std::string lastRequester;
  ndn::mgmt::RejectReply lastRejectReply;
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestCommandAuthenticator, CommandAuthenticatorFixture)

BOOST_AUTO_TEST_CASE(Certs)
{
  Name id0("/localhost/CommandAuthenticator/0");
  Name id1("/localhost/CommandAuthenticator/1");
  Name id2("/localhost/CommandAuthenticator/2");
  BOOST_REQUIRE(m_keyChain.createIdentity(id0));
  BOOST_REQUIRE(saveIdentityCert(id1, "1.ndncert", true));
  BOOST_REQUIRE(saveIdentityCert(id2, "2.ndncert", true));

  makeModules({"module0", "module1", "module2", "module3", "module4", "module5", "module6", "module7"});
  const std::string config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile any
        privileges
        {
          module1
          module3
          module5
          module7
        }
      }
      authorize
      {
        certfile "1.ndncert"
        privileges
        {
          module2
          module3
          module6
          module7
        }
      }
      authorize
      {
        certfile "2.ndncert"
        privileges
        {
          module4
          module5
          module6
          module7
        }
      }
    }
  )CONFIG";
  loadConfig(config);

  // module0: none
  BOOST_CHECK_EQUAL(authorize("module0", id0), false);
  BOOST_CHECK_EQUAL(authorize("module0", id1), false);
  BOOST_CHECK_EQUAL(authorize("module0", id2), false);

  // module1: any
  BOOST_CHECK_EQUAL(authorize("module1", id0), true);
  BOOST_CHECK_EQUAL(authorize("module1", id1), true);
  BOOST_CHECK_EQUAL(authorize("module1", id2), true);

  // module2: id1
  BOOST_CHECK_EQUAL(authorize("module2", id0), false);
  BOOST_CHECK_EQUAL(authorize("module2", id1), true);
  BOOST_CHECK_EQUAL(authorize("module2", id2), false);

  // module3: any,id1
  BOOST_CHECK_EQUAL(authorize("module3", id0), true);
  BOOST_CHECK_EQUAL(authorize("module3", id1), true);
  BOOST_CHECK_EQUAL(authorize("module3", id2), true);

  // module4: id2
  BOOST_CHECK_EQUAL(authorize("module4", id0), false);
  BOOST_CHECK_EQUAL(authorize("module4", id1), false);
  BOOST_CHECK_EQUAL(authorize("module4", id2), true);

  // module5: any,id2
  BOOST_CHECK_EQUAL(authorize("module5", id0), true);
  BOOST_CHECK_EQUAL(authorize("module5", id1), true);
  BOOST_CHECK_EQUAL(authorize("module5", id2), true);

  // module6: id1,id2
  BOOST_CHECK_EQUAL(authorize("module6", id0), false);
  BOOST_CHECK_EQUAL(authorize("module6", id1), true);
  BOOST_CHECK_EQUAL(authorize("module6", id2), true);

  // module7: any,id1,id2
  BOOST_CHECK_EQUAL(authorize("module7", id0), true);
  BOOST_CHECK_EQUAL(authorize("module7", id1), true);
  BOOST_CHECK_EQUAL(authorize("module7", id2), true);
}

BOOST_AUTO_TEST_CASE(Requester)
{
  Name id0("/localhost/CommandAuthenticator/0");
  Name id1("/localhost/CommandAuthenticator/1");
  BOOST_REQUIRE(m_keyChain.createIdentity(id0));
  BOOST_REQUIRE(saveIdentityCert(id1, "1.ndncert", true));

  makeModules({"module0", "module1"});
  const std::string config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile any
        privileges
        {
          module0
        }
      }
      authorize
      {
        certfile "1.ndncert"
        privileges
        {
          module1
        }
      }
    }
  )CONFIG";
  loadConfig(config);

  // module0: any
  BOOST_CHECK_EQUAL(authorize("module0", id0), true);
  BOOST_CHECK_EQUAL(lastRequester, "*");
  BOOST_CHECK_EQUAL(authorize("module0", id1), true);
  BOOST_CHECK_EQUAL(lastRequester, "*");

  // module1: id1
  BOOST_CHECK_EQUAL(authorize("module1", id0), false);
  BOOST_CHECK_EQUAL(authorize("module1", id1), true);
  BOOST_CHECK(id1.isPrefixOf(lastRequester));
}

class IdentityAuthorizedFixture : public CommandAuthenticatorFixture
{
protected:
  IdentityAuthorizedFixture()
  {
    BOOST_REQUIRE(saveIdentityCert(id1, "1.ndncert", true));

    makeModules({"module1"});
    const std::string config = R"CONFIG(
      authorizations
      {
        authorize
        {
          certfile "1.ndncert"
          privileges
          {
            module1
          }
        }
      }
    )CONFIG";
    loadConfig(config);
  }

  bool
  authorize1_V02(const std::function<void(Interest&)>& modifyInterest)
  {
    return authorize("module1", id1, modifyInterest, ndn::security::SignedInterestFormat::V02);
  }

  bool
  authorize1_V03(const std::function<void(Interest&)>& modifyInterest)
  {
    return authorize("module1", id1, modifyInterest, ndn::security::SignedInterestFormat::V03);
  }

protected:
  const Name id1{"/localhost/CommandAuthenticator/1"};
};

BOOST_FIXTURE_TEST_SUITE(Reject, IdentityAuthorizedFixture)

BOOST_AUTO_TEST_CASE(NameTooShort)
{
  BOOST_CHECK_EQUAL(authorize1_V02(
    [] (Interest& interest) {
      interest.setName("/prefix");
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);
}

BOOST_AUTO_TEST_CASE(BadSigInfo)
{
  BOOST_CHECK_EQUAL(authorize1_V02(
    [] (Interest& interest) {
      setNameComponent(interest, ndn::command_interest::POS_SIG_INFO, "not-sig-info");
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);

  BOOST_CHECK_EQUAL(authorize1_V03(
    [] (Interest& interest) {
      auto sigInfo = interest.getSignatureInfo().value();
      sigInfo.addCustomTlv("7F00"_block);
      interest.setSignatureInfo(sigInfo);
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);
}

BOOST_AUTO_TEST_CASE(MissingKeyLocator)
{
  BOOST_CHECK_EQUAL(authorize1_V02(
    [] (Interest& interest) {
      ndn::SignatureInfo sigInfo(interest.getName().at(ndn::command_interest::POS_SIG_INFO).blockFromValue());
      sigInfo.setKeyLocator(std::nullopt);
      setNameComponent(interest, ndn::command_interest::POS_SIG_INFO, span(sigInfo.wireEncode()));
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);

  BOOST_CHECK_EQUAL(authorize1_V03(
    [] (Interest& interest) {
      auto sigInfo = interest.getSignatureInfo().value();
      sigInfo.setKeyLocator(std::nullopt);
      interest.setSignatureInfo(sigInfo);
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);
}

BOOST_AUTO_TEST_CASE(BadKeyLocatorType)
{
  ndn::KeyLocator kl;
  kl.setKeyDigest(ndn::makeBinaryBlock(tlv::KeyDigest, {0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD}));

  BOOST_CHECK_EQUAL(authorize1_V02(
    [&kl] (Interest& interest) {
      ndn::SignatureInfo sigInfo(tlv::SignatureSha256WithEcdsa, kl);
      setNameComponent(interest, ndn::command_interest::POS_SIG_INFO, span(sigInfo.wireEncode()));
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);

  BOOST_CHECK_EQUAL(authorize1_V03(
    [&kl] (Interest& interest) {
      auto sigInfo = interest.getSignatureInfo().value();
      sigInfo.setKeyLocator(kl);
      interest.setSignatureInfo(sigInfo);
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);
}

BOOST_AUTO_TEST_CASE(BadSigValue)
{
  BOOST_CHECK_EQUAL(authorize1_V02(
    [] (Interest& interest) {
      setNameComponent(interest, ndn::command_interest::POS_SIG_VALUE, "bad-signature");
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);

  BOOST_CHECK_EQUAL(authorize1_V03(
    [] (Interest& interest) {
      interest.setSignatureValue({0xBA, 0xAD});
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);
}

BOOST_AUTO_TEST_CASE(MissingTimestamp)
{
  BOOST_CHECK_EQUAL(authorize1_V02(
    [] (Interest& interest) {
      setNameComponent(interest, ndn::command_interest::POS_TIMESTAMP, "not-timestamp");
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);

  BOOST_CHECK_EQUAL(authorize1_V03(
    [] (Interest& interest) {
      auto sigInfo = interest.getSignatureInfo().value();
      sigInfo.setTime(std::nullopt);
      interest.setSignatureInfo(sigInfo);
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);
}

BOOST_AUTO_TEST_CASE(ReplayedTimestamp)
{
  name::Component timestampComp;
  BOOST_CHECK_EQUAL(authorize1_V02(
    [&timestampComp] (const Interest& interest) {
      timestampComp = interest.getName().at(ndn::command_interest::POS_TIMESTAMP);
    }
  ), true); // accept first command
  BOOST_CHECK_EQUAL(authorize1_V02(
    [&timestampComp] (Interest& interest) {
      setNameComponent(interest, ndn::command_interest::POS_TIMESTAMP, timestampComp);
    }
  ), false); // reject second command because timestamp equals first command
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);

  time::system_clock::time_point tp;
  BOOST_CHECK_EQUAL(authorize1_V03(
    [&tp] (const Interest& interest) {
      tp = interest.getSignatureInfo().value().getTime().value();
    }
  ), true); // accept first command
  BOOST_CHECK_EQUAL(authorize1_V03(
    [&tp] (Interest& interest) {
      auto sigInfo = interest.getSignatureInfo().value();
      sigInfo.setTime(tp);
      interest.setSignatureInfo(sigInfo);
    }
  ), false); // reject second command because timestamp equals first command
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);
}

BOOST_AUTO_TEST_CASE(NotAuthorized)
{
  Name id0("/localhost/CommandAuthenticator/0");
  BOOST_REQUIRE(m_keyChain.createIdentity(id0));

  BOOST_CHECK_EQUAL(authorize("module1", id0), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);
}

BOOST_FIXTURE_TEST_CASE(MissingAuthorizationsSection, CommandAuthenticatorFixture)
{
  Name id0("/localhost/CommandAuthenticator/0");
  BOOST_REQUIRE(m_keyChain.createIdentity(id0));

  makeModules({"module42"});
  loadConfig("");

  BOOST_CHECK_EQUAL(authorize("module42", id0), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);
}

BOOST_AUTO_TEST_SUITE_END() // Reject

BOOST_AUTO_TEST_SUITE(BadConfig)

BOOST_AUTO_TEST_CASE(EmptyAuthorizationsSection)
{
  const std::string config = R"CONFIG(
    authorizations
    {
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(UnrecognizedKey)
{
  const std::string config = R"CONFIG(
    authorizations
    {
      unrecognized_key
      {
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(CertfileMissing)
{
  const std::string config = R"CONFIG(
    authorizations
    {
      authorize
      {
        privileges
        {
        }
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(CertUnreadable)
{
  const std::string config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile "1.ndncert"
        privileges
        {
        }
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(PrivilegesMissing)
{
  const std::string config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile any
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(UnregisteredModule)
{
  const std::string config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile any
        privileges
        {
          nosuchmodule
        }
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // BadConfig

BOOST_AUTO_TEST_SUITE_END() // TestCommandAuthenticator
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace nfd::tests
