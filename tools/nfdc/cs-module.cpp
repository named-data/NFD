/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "cs-module.hpp"
#include "format-helpers.hpp"

#include <ndn-cxx/util/indented-stream.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

void
CsModule::registerCommands(CommandParser& parser)
{
  CommandDefinition defCsConfig("cs", "config");
  defCsConfig
    .setTitle("change CS configuration")
    .addArg("capacity", ArgValueType::UNSIGNED, Required::NO, Positional::NO)
    .addArg("admit", ArgValueType::BOOLEAN, Required::NO, Positional::NO)
    .addArg("serve", ArgValueType::BOOLEAN, Required::NO, Positional::NO);
  parser.addCommand(defCsConfig, &CsModule::config);
}

void
CsModule::config(ExecuteContext& ctx)
{
  using boost::logic::indeterminate;

  auto capacity = ctx.args.getOptional<uint64_t>("capacity");
  auto enableAdmit = ctx.args.getTribool("admit");
  auto enableServe = ctx.args.getTribool("serve");

  ControlParameters p;
  if (capacity) {
    p.setCapacity(*capacity);
  }
  if (!indeterminate(enableAdmit)) {
    p.setFlagBit(ndn::nfd::BIT_CS_ENABLE_ADMIT, enableAdmit);
  }
  if (!indeterminate(enableServe)) {
    p.setFlagBit(ndn::nfd::BIT_CS_ENABLE_SERVE, enableServe);
  }

  ctx.controller.start<ndn::nfd::CsConfigCommand>(p,
    [&] (const ControlParameters& resp) {
      text::ItemAttributes ia;
      ctx.out << "cs-config-updated "
              << ia("capacity") << resp.getCapacity()
              << ia("admit") << text::OnOff{resp.getFlagBit(ndn::nfd::BIT_CS_ENABLE_ADMIT)}
              << ia("serve") << text::OnOff{resp.getFlagBit(ndn::nfd::BIT_CS_ENABLE_SERVE)}
              << '\n';
    },
    ctx.makeCommandFailureHandler("updating CS config"),
    ctx.makeCommandOptions());

  ctx.face.processEvents();
}

void
CsModule::fetchStatus(Controller& controller,
                      const std::function<void()>& onSuccess,
                      const Controller::DatasetFailCallback& onFailure,
                      const CommandOptions& options)
{
  controller.fetch<ndn::nfd::CsInfoDataset>(
    [this, onSuccess] (const CsInfo& result) {
      m_status = result;
      onSuccess();
    },
    onFailure, options);
}

void
CsModule::formatStatusXml(std::ostream& os) const
{
  formatItemXml(os, m_status);
}

void
CsModule::formatItemXml(std::ostream& os, const CsInfo& item)
{
  os << "<cs>";
  os << "<capacity>" << item.getCapacity() << "</capacity>";
  os << xml::Flag{"admitEnabled", item.getEnableAdmit()};
  os << xml::Flag{"serveEnabled", item.getEnableServe()};
  os << "<nEntries>" << item.getNEntries() << "</nEntries>";
  os << "<nHits>" << item.getNHits() << "</nHits>";
  os << "<nMisses>" << item.getNMisses() << "</nMisses>";
  os << "</cs>";
}

void
CsModule::formatStatusText(std::ostream& os) const
{
  os << "CS information:\n";
  ndn::util::IndentedStream indented(os, "  ");
  formatItemText(indented, m_status);
}

void
CsModule::formatItemText(std::ostream& os, const CsInfo& item)
{
  text::ItemAttributes ia(true, 8);
  os << ia("capacity") << item.getCapacity()
     << ia("admit") << text::OnOff{item.getEnableAdmit()}
     << ia("serve") << text::OnOff{item.getEnableServe()}
     << ia("nEntries") << item.getNEntries()
     << ia("nHits") << item.getNHits()
     << ia("nMisses") << item.getNMisses()
     << ia.end();
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
