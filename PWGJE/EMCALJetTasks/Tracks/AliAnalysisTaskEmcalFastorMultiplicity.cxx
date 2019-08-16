/************************************************************************************
 * Copyright (C) 2019, Copyright Holders of the ALICE Collaboration                 *
 * All rights reserved.                                                             *
 *                                                                                  *
 * Redistribution and use in source and binary forms, with or without               *
 * modification, are permitted provided that the following conditions are met:      *
 *     * Redistributions of source code must retain the above copyright             *
 *       notice, this list of conditions and the following disclaimer.              *
 *     * Redistributions in binary form must reproduce the above copyright          *
 *       notice, this list of conditions and the following disclaimer in the        *
 *       documentation and/or other materials provided with the distribution.       *
 *     * Neither the name of the <organization> nor the                             *
 *       names of its contributors may be used to endorse or promote products       *
 *       derived from this software without specific prior written permission.      *
 *                                                                                  *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND  *
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED    *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
 * DISCLAIMED. IN NO EVENT SHALL ALICE COLLABORATION BE LIABLE FOR ANY              *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES       *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;     *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND      *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS    *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                     *
 ************************************************************************************/
#include <iostream>

#include <THistManager.h>

#include "AliAnalysisManager.h"
#include "AliAnalysisTaskEmcalFastorMultiplicity.h"
#include "AliInputEventHandler.h"
#include "AliLog.h"
#include "AliMultSelection.h"
#include "AliMultEstimator.h"
#include "AliVEvent.h"
#include "AliVCaloTrigger.h"

ClassImp(PWGJE::EMCALJetTasks::AliAnalysisTaskEmcalFastorMultiplicity);

using namespace PWGJE::EMCALJetTasks;

AliAnalysisTaskEmcalFastorMultiplicity::AliAnalysisTaskEmcalFastorMultiplicity() : AliAnalysisTaskEmcal(),
                                                                                   fHistos(nullptr)
{
}

AliAnalysisTaskEmcalFastorMultiplicity::AliAnalysisTaskEmcalFastorMultiplicity(const char *name) : AliAnalysisTaskEmcal(name, kTRUE),
                                                                                                   fHistos(nullptr)
{
}

AliAnalysisTaskEmcalFastorMultiplicity::~AliAnalysisTaskEmcalFastorMultiplicity()
{
  if (fHistos)
    delete fHistos;
}

void AliAnalysisTaskEmcalFastorMultiplicity::UserCreateOutputObjects()
{
  AliAnalysisTaskEmcal::UserCreateOutputObjects();

  const std::array<int, 15> thresholds = {{0, 1, 2, 3, 4, 5, 10, 15, 20, 30, 40, 50, 60, 80, 100}};
  fHistos = new THistManager(Form("%s_histos"));
  for (auto t : thresholds)
  {
    fHistos->CreateTH2(Form("hFastorMultL0_ADC%d", t), Form("Multiplicity of FastORs with ADC >= %d at L0", t), 100, 0., 100, 6500, 0., 6500);
    fHistos->CreateTH2(Form("hFastorMultL1_ADC%d", t), Form("Multiplicity of FastORs with ADC >= %d at L1", t), 100, 0., 100, 6500, 0., 6500);
  }

  for (auto h : *fHistos->GetListOfHistograms())
    fOutput->Add(h);

  PostData(1, fOutput);
}

bool AliAnalysisTaskEmcalFastorMultiplicity::IsTriggerSelected()
{
  // Select min. bias evetns for the moment
  return (fInputHandler->IsEventSelected() & AliVEvent::kINT7);
}

bool AliAnalysisTaskEmcalFastorMultiplicity::Run()
{
  auto mult = dynamic_cast<AliMultSelection *>(InputEvent()->FindListObject("MultSelection"));
  if (!mult)
  {
    AliErrorStream() << "Multiplicity object not available from event" << std::endl;
    return false;
  }
  auto multval = mult->GetEstimator("V0A")->GetPercentile();

  std::map<int, int> countersL0, countersL1;
  const std::array<int, 15> thresholds = {{0, 1, 2, 3, 4, 5, 10, 15, 20, 30, 40, 50, 60, 80, 100}};
  for (auto t : thresholds)
  {
    countersL0[t] = 0;
    countersL1[t] = 0;
  }

  auto triggers = fInputEvent->GetCaloTrigger("EMCAL");
  triggers->Reset();
  while (triggers->Next())
  {
    Float_t adcL0;
    triggers->GetAmplitude(adcL0);
    auto adcL1 = triggers->GetL1TimeSum();
    if (adcL1 < 0)
      adcL1 = 0;
    for (auto t : thresholds)
    {
      if (adcL0 >= t)
        countersL0[t]++;
      if (adcL1 >= t)
        countersL1[t]++;
    }
  }

  for (auto t : thresholds)
  {
    fHistos->FillTH2(Form("hFastorMultL0_ADC%d", t), multval, countersL0[t]);
    fHistos->FillTH2(Form("hFastorMultL0_ADC%d", t), multval, countersL1[t]);
  }

  return true;
}

AliAnalysisTaskEmcalFastorMultiplicity *AliAnalysisTaskEmcalFastorMultiplicity::AddTaskEmcalFastorMultiplicity(const char *name)
{
  auto mgr = AliAnalysisManager::GetAnalysisManager();
  if (!mgr)
  {
    AliErrorGeneralStream("AliAnalysisTaskEmcalFastorMultiplicity::AddTaskEmcalFastorMultiplicity") << "No Analysis Manager defined" << std::endl;
    return nullptr;
  }

  auto task = new AliAnalysisTaskEmcalFastorMultiplicity(name);
  mgr->AddTask(task);

  mgr->ConnectInput(task, 0, mgr->GetCommonInputContainer());
  mgr->ConnectOutput(task, 1, mgr->CreateContainer(Form("HistosFastorMultiplicity_%s", name), TList::Class(), AliAnalysisManager::kOutputContainer, mgr->GetCommonFileName()));
  return task;
}