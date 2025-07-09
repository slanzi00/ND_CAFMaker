#include "TMSRecoBranchFiller.h"
#include "truth/FillTruth.h"

/*
 * Liam O'Sullivan <liam.osullivan@uni-mainz.de>  -  Oct 2024
 * Put together mostly from the previous example and the MINERvA RecoBranchFiller
 */

namespace cafmaker
{

  TMSRecoBranchFiller::TMSRecoBranchFiller(const std::string &tmsRecoFilename)
    : IRecoBranchFiller("TMS"),
    fTriggers(),
    fLastTriggerReqd(fTriggers.end())
  {
    fTMSRecoFile = new TFile(tmsRecoFilename.c_str(), "READ");
    name = std::string("TMS");

    if (!fTMSRecoFile->IsZombie()) {
      SetConfigured(true);

      // Save pointer to input tree
      TMSRecoTree = dynamic_cast<TTree*>(fTMSRecoFile->Get("Reco_Tree"));
      if (!TMSRecoTree) {
        std::cerr << "Did not find TMS reco tree Reco_Tree in input file " << tmsRecoFilename << std::endl;
        std::cerr << "Are you sure this is a TMS reco file?" << std::endl;
        throw;
      }

      TMSRecoTree->SetBranchAddress("EventNo",               &_EventNo);
      TMSRecoTree->SetBranchAddress("SliceNo",               &_SliceNo);
      TMSRecoTree->SetBranchAddress("SpillNo",               &_SpillNo);
      TMSRecoTree->SetBranchAddress("nTracks",               &_nTracks);
      TMSRecoTree->SetBranchAddress("nHits",                 _nHitsInTrack);
      TMSRecoTree->SetBranchAddress("Length",                _TrackLength);
      TMSRecoTree->SetBranchAddress("Momentum",              _TrackMomentum);
      //TMSRecoTree->SetBranchAddress("Charge",                _TrackCharge); // TODO: Uncomment when Occupancy filled by TMS
      TMSRecoTree->SetBranchAddress("EnergyRange",           _TrackTotalEnergy);
      TMSRecoTree->SetBranchAddress("EnergyDeposit",         _TrackEnergyDeposit);
      //TMSRecoTree->SetBranchAddress("Occupancy",             _Occupancy); // TODO: Uncomment when Occupancy filled by TMS

      TMSRecoTree->SetBranchAddress("TrackHitPos",            _TrackRecoHitPos);
      TMSRecoTree->SetBranchAddress("StartPos",              _TrackStartPos);
      TMSRecoTree->SetBranchAddress("KalmanPos",             _TrackHitPos);
      TMSRecoTree->SetBranchAddress("EndPos",                _TrackEndPos);
      TMSRecoTree->SetBranchAddress("StartDirection",        _TrackStartDirection);
      TMSRecoTree->SetBranchAddress("EndDirection",          _TrackEndDirection);
    } else {
      fTMSRecoFile = NULL;
      TMSRecoTree  = NULL;
      std::cerr << "The TMS reco file you provided: " << tmsRecoFilename 
                << " appears to be a Zombie 🧟" << std::endl;
      throw;
    }
  }


  TMSRecoBranchFiller::~TMSRecoBranchFiller() {
    delete TMSRecoTree;
    fTMSRecoFile->Close();
    delete fTMSRecoFile;
    TMSRecoTree = NULL;
    fTMSRecoFile = NULL;
  }

  // ---------------------------------------------------------------------------

  // here we copy all the TMS reco into the SRTMS branch of the StandardRecord object.
  void TMSRecoBranchFiller::_FillRecoBranches(const Trigger &trigger,
                                              caf::StandardRecord &sr,
                                              const cafmaker::Params &par,
                                              const TruthMatcher *truthMatcher) const
  {

    sr.meta.tms.enabled = true;

    // Nicked from the MINVERvA example:
    // figure out where in our list of triggers this event index is.
    // we should always be looking forwards, since we expect to be traversing in that direction
    auto it_start = (fLastTriggerReqd == fTriggers.end()) ? fTriggers.cbegin() : fLastTriggerReqd;
    auto itTrig = std::find(it_start, fTriggers.cend(), trigger);
    if (itTrig == fTriggers.end())
    {
      LOG.FATAL() << "Reco branch filler '" << GetName() << "' could not find trigger with evtID == " << trigger.evtID << "!  Abort.\n";
      abort();
    }
    std::size_t idx = std::distance(fTriggers.cbegin(), itTrig);
    LOG.VERBOSE() << "    Reco branch filler '" << GetName() << "', trigger.evtID == " << trigger.evtID << ", internal evt idx = " << idx << ".\n";

    int i = trigger.evtID; // pseudo-itterator for ixn
    // Get nth entry from tree

    int LastSpillNo = -999999; //_SpillNo;
    TMSRecoTree->GetEntry(i); // Load first entry for now
    LastSpillNo = _SpillNo;

    sr.nd.tms.ixn.emplace_back();
    caf::SRTMSInt& interaction = sr.nd.tms.ixn.back();

    interaction.ntracks = 0;
    while (_SpillNo == LastSpillNo && i < TMSRecoTree->GetEntries()) // while we're in the spill
    {
      TMSRecoTree->GetEntry(i++); // Load each subsequent entry in the spill, start from original i
      if (_nTracks > 0)
      {
        interaction.tracks.resize(_nTracks + interaction.tracks.size());
        for (int j = 0; j < _nTracks; ++j) {
          interaction.ntracks++;
          interaction.tracks[j].start   = caf::SRVector3D(_TrackStartPos[j][0]/10., _TrackStartPos[j][1]/10., _TrackStartPos[j][2]/10.);;
          interaction.tracks[j].end     = caf::SRVector3D(_TrackEndPos[j][0]/10., _TrackEndPos[j][1]/10., _TrackEndPos[j][2]/10.);
          interaction.tracks[j].dir     = caf::SRVector3D(_TrackStartDirection[j][0], _TrackStartDirection[j][1] , _TrackStartDirection[j][2]);
          interaction.tracks[j].enddir  = caf::SRVector3D(_TrackEndDirection[j][0], _TrackEndDirection[j][1] , _TrackEndDirection[j][2]);

          // Calculate length by summing up the distances from the kalman reco positions
//          double tmpLength_cm = 0.0;
//          for (int k=0; k<_nHitsInTrack[j]-1; k++)
//            tmpLength_cm += sqrt( pow(_TrackRecoHitPos[j][k][0] - _TrackRecoHitPos[j][k+1][0], 2)
//                                + pow(_TrackRecoHitPos[j][k][1] - _TrackRecoHitPos[j][k+1][1], 2)
//                                + pow(_TrackRecoHitPos[j][k][2] - _TrackRecoHitPos[j][k+1][2], 2) );

          // Track info
          //interaction.tracks[j].len_cm    = tmpLength_cm; //trackVec->Mag();
          interaction.tracks[j].len_gcm2  = (_TrackLength[j]>0.0) ? _TrackLength[j]/10. : 0.0; // idk why we have negatives
          interaction.tracks[j].qual      = _Occupancy[j]; // TODO: Apparently this is a "track quality", nominally (hits in track)/(total hits)
          interaction.tracks[j].Evis      = _TrackEnergyDeposit[j];
        }
      }
    }
  }


  void TMSRecoBranchFiller::FillTrueInteraction(caf::SRTrueInteraction & srTrueInt,
                                                    int int_id) const
  {
    LOG.DEBUG() << "    now copying truth info from Mnv TrueInteraction to SRTrueInteraction...\n";

    const auto NaN = std::numeric_limits<float>::signaling_NaN();

    // here we are converting from mm (units from MINERvA) to cm
    ValidateOrCopy(mc_int_vtx[int_id][0]/10., srTrueInt.vtx.x, NaN, "SRTrueInteraction::vtx::x");
    ValidateOrCopy(mc_int_vtx[int_id][1]/10., srTrueInt.vtx.y, NaN, "SRTrueInteraction::vtx::y");
    ValidateOrCopy(mc_int_vtx[int_id][2]/10., srTrueInt.vtx.z, NaN, "SRTrueInteraction::vtx::z");


  }

  void TMSRecoBranchFiller::FillTrueParticle(caf::SRTrueParticle & srTruePart,
                                                 int max_trkid) const
  {
    const auto NaN = std::numeric_limits<float>::signaling_NaN();
    ValidateOrCopy(mc_traj_pdg[max_trkid], srTruePart.pdg, 0, "pdg_code");

    ValidateOrCopy(mc_traj_edepsim_trkid[max_trkid], srTruePart.G4ID, -1, "SRTrueParticle::track_id");
    ValidateOrCopy(mc_traj_parentid[max_trkid], srTruePart.parent, -1, "SRTrueParticle::parent");
    ValidateOrCopy(mc_traj_point_px[max_trkid][0]/1000., srTruePart.p.px, NaN, "SRTrueParticle::p.px");
    ValidateOrCopy(mc_traj_point_py[max_trkid][0]/1000., srTruePart.p.py, NaN, "SRTrueParticle::p.py");
    ValidateOrCopy(mc_traj_point_pz[max_trkid][0]/1000., srTruePart.p.pz, NaN, "SRTrueParticle::p.pz");

    try
    {
      ValidateOrCopy(mc_traj_point_E[max_trkid][0] / 1000., srTruePart.p.E, NaN, "SRTrueParticle::p.E");
    }
    catch (std::runtime_error & e)
    {
      auto diff = (mc_traj_point_E[max_trkid][0] / 1000. - srTruePart.p.E);
      if (diff < 1) // < 1 MeV
      {
      LOG.WARNING() << "True particle energy (track id=" << srTruePart.G4ID << ", pdg=" << srTruePart.pdg << ", stored E=" << srTruePart.p.E << ")"
                      << " differs by " << diff << " MeV between stored (GENIE?) and ML-reco pass-through values";
      }
      else
        throw e;
    }
  }

  std::deque<Trigger> TMSRecoBranchFiller::GetTriggers(int triggerType, bool beamOnly) const
  {
    std::deque<Trigger> triggers;
    int lastSpillNo = -99999999;

    if (fTriggers.empty())
    {
      LOG.DEBUG() << "Loading triggers with type " << triggerType << " within branch filler '" << GetName() << "' from " << TMSRecoTree->GetEntries() << " TMS Reco_Tree:\n";
      fTriggers.reserve(TMSRecoTree->GetEntries());

      for (int entry = 0; entry < TMSRecoTree->GetEntries(); entry++)
      {
        TMSRecoTree->GetEntry(entry);

        if (_SpillNo == lastSpillNo)
          continue; // Only first 'event' in each spill populates a trigger

        lastSpillNo = _SpillNo;

        Trigger & prev_trig = fTriggers.back(); // trigger before 'trig'
        fTriggers.emplace_back();               // add new trigger entry (unfilled)
        Trigger & trig      = fTriggers.back(); // trigger we're working on

        trig.evtID = entry;
        trig.triggerType = 0; //2147483647; // TODO real number?

        if (entry == 0) // TODO do this less bad
          trig.triggerTime_ns = 0;
        else
          trig.triggerTime_ns = prev_trig.triggerTime_ns + 2E8 ;

        if (entry == 0) // TODO do this less bad
          trig.triggerTime_s = 0;
        else
        {
          trig.triggerTime_s = prev_trig.triggerTime_s + 1; // TODO: Pull the 1.2 from correct place in file
          if (trig.triggerTime_ns >= 1E9)
          {
            trig.triggerTime_s += 1;
            trig.triggerTime_ns -= 1E9;
          }
        }

        LOG.VERBOSE() << "  added trigger:  evtID=" << trig.evtID
                      << ", triggerType=" << trig.triggerType
                      << ", triggerTime_s=" << trig.triggerTime_s
                      << ", triggerTime_ns=" << trig.triggerTime_ns
                      << "\n";
      }
      fLastTriggerReqd = fTriggers.end();  // since we just modified the list, any iterators have been invalidated
    }

    for (const Trigger & trigger : fTriggers)
    {
      if (triggerType < 0 || triggerType == fTriggers.back().triggerType)
      {
        triggers.push_back(trigger);
      }
    }

    return triggers;
  }

} // end namespace
