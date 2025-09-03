#include "NDLArTMSUniqueMatchRecoFiller.h"
#include <cmath>

namespace cafmaker
{
  bool Track_match_sorter(const caf::SRNDTrackAssn trackMatch1, const caf::SRNDTrackAssn trackMatch2) {
    double fScore1 = trackMatch1.matchScore;
    double fScore2 = trackMatch2.matchScore;

    if (fScore1 < fScore2) {
      return true;
    }
    else {
      return false;
    }
  }

  NDLArTMSUniqueMatchRecoFiller::NDLArTMSUniqueMatchRecoFiller(const double sigmaX, const double sigmaY, const bool singleAngle, const double sigmaTh, const double sigmaThX, const double sigmaThY, const bool useTime, const double meanT, const double sigmaT, const double fCut)
    : IRecoBranchFiller("LArTMSMatcher")
  {
    sigma_x = sigmaX;
    sigma_y = sigmaY;
    single_angle = singleAngle;
    sigma_angle = sigmaTh;
    sigma_angle_x = sigmaThX;
    sigma_angle_y = sigmaThY;
    use_time = useTime;
    mean_t = meanT;
    sigma_t = sigmaT;
    f_cut = fCut;
    // nothing to do
    SetConfigured(true);
  }

  std::vector<double> NDLArTMSUniqueMatchRecoFiller::Project_track(const caf::SRTrack track, const bool forward) const
  {
    double x, y, z;

    double dir_x, dir_y, dir_z;
    
    double proj_z;
    double proj_x;
    double proj_y;

    if (forward) { // projects a LAr track forward to TMS
      x = track.end.x;
       y = track.end.y;
       z = track.end.z;

       dir_x = track.enddir.x;
       dir_y = track.enddir.y;
       dir_z = track.enddir.z;

       proj_z = tms_z_lim1 - z;
       proj_x = dir_x*proj_z/dir_z + x;
       proj_y = dir_y*proj_z/dir_z + y;
    }
    else { // projects a TMS track backward to LAr
       x = track.start.x;
       y = track.start.y;
       z = track.start.z;

       dir_x = track.dir.x;
       dir_y = track.dir.y;
       dir_z = track.dir.z;

       proj_z = z - lar_z_lim2;
       proj_x = -dir_x*proj_z/dir_z + x;
       proj_y = -dir_y*proj_z/dir_z + y;
    }
    std::vector<double> proj_point = {proj_x, proj_y, proj_z};
    return proj_point;
  }

  std::vector<double> NDLArTMSUniqueMatchRecoFiller::Angle_between_tracks(const caf::SRTrack tms_track, const caf::SRTrack lar_track) const
  {
      double tms_dir_x = tms_track.dir.x;
      double tms_dir_y = tms_track.dir.y;
      double tms_dir_z = tms_track.dir.z;

      double lar_dir_x = lar_track.enddir.x;
      double lar_dir_y = lar_track.enddir.y;
      double lar_dir_z = lar_track.enddir.z;

      double xz_dot_prod = tms_dir_x*lar_dir_x + tms_dir_z*lar_dir_z;
      if (xz_dot_prod != 0) {
        double xz_dot_prod = xz_dot_prod/(sqrt(pow(tms_dir_x,2)+pow(tms_dir_z,2))*sqrt(pow(lar_dir_x,2)+pow(lar_dir_z,2)));
      }
      double yz_dot_prod = tms_dir_y*lar_dir_y + tms_dir_z*lar_dir_z;
      if (yz_dot_prod != 0) {
        double yz_dot_prod = yz_dot_prod/(sqrt(pow(tms_dir_y,2)+pow(tms_dir_z,2))*sqrt(pow(lar_dir_y,2)+pow(lar_dir_z,2)));
      }
      double dot_prod = tms_dir_x*lar_dir_x + tms_dir_y*lar_dir_y + tms_dir_z*lar_dir_z;
      double angle_x = 180.0/TMath::Pi() * acos(xz_dot_prod);
      double angle_y = 180.0/TMath::Pi() * acos(yz_dot_prod);
      double angle_overall = 180.0/TMath::Pi() * acos(dot_prod);
      std::vector<double> angles = {angle_x,angle_y,angle_overall};
      return angles;
  }

  bool NDLArTMSUniqueMatchRecoFiller::Consider_TMS_track(const caf::SRTrack tms_track, const double tms_z_cutoff) const
  {
    double x_start = tms_track.start.x;
    double y_start = tms_track.start.y;
    double z_start = tms_track.start.z;

    if ((x_start > tms_x_lim1)&&(x_start < tms_x_lim2) &&
        (y_start > tms_y_lim1)&&(y_start < tms_y_lim2) &&
        (z_start > tms_z_lim1)&&(z_start < tms_z_lim1 + tms_z_cutoff) && // checks track begins within fiducial volume and close enough to front
      
        (Project_track(tms_track,false)[0] > lar_x_lim1)&&(Project_track(tms_track,false)[0] < lar_x_lim2) &&
        (Project_track(tms_track,false)[1] > lar_y_lim1)&&(Project_track(tms_track,false)[1] < lar_y_lim2)) // checks that direction would have allowed it to originate from LAr
          {
            return true;
          } 
    else {
      return false;
    }
  }

  bool NDLArTMSUniqueMatchRecoFiller::Consider_LAr_track(const caf::SRTrack lar_track, const double lar_z_cutoff) const
  {
    double x_start = lar_track.start.x;
    double y_start = lar_track.start.y;
    double z_start = lar_track.start.z;

    double x_end = lar_track.end.x;
    double y_end = lar_track.end.y;
    double z_end = lar_track.end.z;

    if ((x_start > lar_x_lim1)&&(x_start < lar_x_lim2) &&
        (y_start > lar_y_lim1)&&(y_start < lar_y_lim2) &&
        (z_start > lar_z_lim1)&&(z_start < lar_z_lim2) && // checks track begins within fiducial volume

        (x_end > lar_x_lim1)&&(x_end < lar_x_lim2) &&
        (y_end > lar_y_lim1)&&(y_end < lar_y_lim2) &&
        (z_end > lar_z_lim2 - lar_z_cutoff)&&(z_end < lar_z_lim2) && // checks track ends close enough to back of LAr
      
        (Project_track(lar_track,true)[0] > tms_x_lim1)&&(Project_track(lar_track,true)[0] < tms_x_lim2) &&
        (Project_track(lar_track,true)[1] > tms_y_lim1)&&(Project_track(lar_track,true)[1] < tms_y_lim2)) // checks that direction would allow it to hit TMS
        {
          return true;
        } 
    else {
      return false;
    }
  }

  void
  NDLArTMSUniqueMatchRecoFiller::_FillRecoBranches(const Trigger &trigger,
                                             caf::StandardRecord &sr,
                                             const cafmaker::Params &par,
                                             const TruthMatcher *truthMatcher) const
  {
    std::vector<caf::SRNDTrackAssn> possiblePandoraMatches; // vector will store all possible matched tracks between Pandora and TMS
    std::vector<caf::SRNDTrackAssn> possibleSPINEMatches; // vector will store all possible matched tracks between SPINE and TMS

    double tms_z_cutoff = 20;
    double lar_z_cutoff = 20; // tracks must overlap last/first 20 cm of the detectors
    
    for (unsigned int ixn_tms = 0; ixn_tms < sr.nd.tms.nixn; ixn_tms++)
    {
      caf::SRTMSInt tms_int = sr.nd.tms.ixn[ixn_tms];
      unsigned int n_tms_tracks = tms_int.ntracks;
      

      for (unsigned int itms = 0; itms < n_tms_tracks; itms++)
      {
        caf::SRTrack tms_trk = tms_int.tracks[itms];
        double tms_time = tms_trk.time;

        if (!Consider_TMS_track(tms_trk,tms_z_cutoff)) {
          continue; //skips the tms track if it isn't suitable according to the function
        }

        for (unsigned int ixn_pan = 0; ixn_pan < sr.nd.lar.npandora; ixn_pan++)
        {
          caf::SRNDLArInt pan_int = sr.nd.lar.pandora[ixn_pan];
          unsigned int n_pan_tracks = pan_int.ntracks;

          for (unsigned int ipan = 0; ipan < n_pan_tracks; ipan++)
          {
            caf::SRTrack pan_trk = pan_int.tracks[ipan];

            if (!Consider_LAr_track(pan_trk,lar_z_cutoff)) {
              continue; //skips the lar track if it isn't suitable according to the function
            }
            
            std::vector<double> proj_vec = Project_track(pan_trk,true);

            double delta_x = tms_trk.start.x - proj_vec[0];
            double delta_y = tms_trk.start.y - proj_vec[1];

            std::vector<double> angles = Angle_between_tracks(tms_trk,pan_trk);

            double fScore = std::numeric_limits<double>::max();

            double lar_time = 0;
            caf::SRVector3D start_pos;
            double delta_t = 0;

            if (single_angle) {
              double angle = angles[2];
              fScore = pow(delta_x/sigma_x,2) + pow(delta_y/sigma_y,2) + pow(angle/sigma_angle,2);
            }
            else {
              double angle_x = angles[0];
              double angle_y = angles[1];
              fScore = pow(delta_x/sigma_x,2) + pow(delta_y/sigma_y,2) + pow(angle_x/sigma_angle_x,2)+ pow(angle_y/sigma_angle_y,2);
            }
            if (use_time) {
              std::vector<float> tO = pan_trk.truthOverlap;
              std::vector<caf::TrueParticleID> truIDs = pan_trk.truth;
              int idx_max = std::distance(tO.begin(),std::max_element(tO.begin(),tO.end()));
              caf::TrueParticleID partID = truIDs[idx_max];
              const auto& matchedPart = sr.mc.Particle(partID);
              if (matchedPart != nullptr) {
                lar_time = matchedPart->time - 1e9*trigger.triggerTime_s - trigger.triggerTime_ns;
                start_pos = matchedPart->start_pos;
                delta_t = lar_time - tms_time;
                fScore += pow((delta_t-mean_t)/sigma_t,2);
              }
            }

            caf::SRTMSID tmsid;
            tmsid.ixn = ixn_tms;
            tmsid.idx = itms;
            caf::SRNDLArID panid;
            panid.reco = caf::kPandoraNDLAr;
            panid.ixn = ixn_pan;
            panid.idx = ipan;

            caf::SRNDTrackAssn potential_match;
            if (use_time) {
              potential_match.matchType = caf::NDRecoMatchType::kUniqueWithTime;
            }
            else {
              potential_match.matchType = caf::MatchType::kUniqueNoTime;
            }
            potential_match.tmsid = tmsid;
            potential_match.larid = panid;
            potential_match.matchScore = fScore;
            potential_match.transdispl = sqrt(pow(delta_x,2)+pow(delta_y,2));
            potential_match.angdispl = cos(TMath::Pi()/180.0 * angles[2]);

            caf::SRTrack joint_track = potential_match.trk;
            joint_track.start = pan_trk.start;
            joint_track.end = tms_trk.end;
            joint_track.dir = pan_trk.dir;
            joint_track.enddir = tms_trk.enddir;

            joint_track.time = tms_trk.time; // TODO: once we have LAr time working properly this should be switched to pan_trk.time

            joint_track.Evis = pan_trk.Evis + tms_trk.Evis;
            // TODO: add the rest of the joint_track attributes
            
            possiblePandoraMatches.push_back(potential_match);
          }
        }

        for (unsigned int ixn_dlp = 0; ixn_dlp < sr.nd.lar.ndlp; ixn_dlp++)
        {
          caf::SRNDLArInt dlp_int = sr.nd.lar.dlp[ixn_dlp];
          unsigned int n_dlp_tracks = dlp_int.ntracks;

          for (unsigned int idlp = 0; idlp < n_dlp_tracks; idlp++)
          {
            caf::SRTrack dlp_trk = dlp_int.tracks[idlp];

            if (!Consider_LAr_track(dlp_trk,lar_z_cutoff)) {
              continue; //skips the lar track if it isn't suitable according to the function
            }

            std::vector<double> proj_vec = Project_track(dlp_trk,true);

            double delta_x = tms_trk.start.x - proj_vec[0];
            double delta_y = tms_trk.start.y - proj_vec[1];

            std::vector<double> angles = Angle_between_tracks(tms_trk,dlp_trk);
            double fScore = std::numeric_limits<double>::max();

            if (single_angle) {
              double angle = angles[2];
              fScore = pow(delta_x/sigma_x,2) + pow(delta_y/sigma_y,2) + pow(angle/sigma_angle,2);
            }
            else {
              double angle_x = angles[0];
              double angle_y = angles[1];
              fScore = pow(delta_x/sigma_x,2) + pow(delta_y/sigma_y,2) + pow(angle_x/sigma_angle_x,2)+ pow(angle_y/sigma_angle_y,2);
            }
            if (use_time) {
              std::vector<float> tO = dlp_trk.truthOverlap;
              std::vector<caf::TrueParticleID> truIDs = dlp_trk.truth;
              int idx_max = std::distance(tO.begin(),std::max_element(tO.begin(),tO.end()));
              // Finds the index of the TrueParticleID that was responsible for the largest portion of the track
              caf::TrueParticleID partID = truIDs[idx_max];
              const auto& matchedPart = sr.mc.Particle(partID);
              double lar_time = 0;
              if (matchedPart != nullptr) {
                lar_time = matchedPart->time - 1e9*trigger.triggerTime_s - trigger.triggerTime_ns;
                double delta_t = lar_time - tms_time;
                fScore += pow((delta_t-mean_t)/sigma_t,2);
              }
            }

            caf::SRTMSID tmsid;
            tmsid.ixn = ixn_tms;
            tmsid.idx = itms;
            caf::SRNDLArID dlpid;
            dlpid.reco = caf::kDeepLearnPhys;
            dlpid.ixn = ixn_dlp;
            dlpid.idx = idlp;

            caf::SRNDTrackAssn potential_match;
            if (use_time) {
              potential_match.matchType = caf::MatchType::kUniqueWithTime;
            }
            else {
              potential_match.matchType = caf::MatchType::kUniqueNoTime;
            }
            potential_match.tmsid = tmsid;
            potential_match.larid = dlpid;
            potential_match.matchScore = fScore;
            potential_match.transdispl = sqrt(pow(delta_x,2)+pow(delta_y,2));
            potential_match.angdispl = cos(TMath::Pi()/180.0 * angles[2]);

            caf::SRTrack joint_track = potential_match.trk;
            joint_track.start = dlp_trk.start;
            joint_track.end = tms_trk.end;
            joint_track.dir = dlp_trk.dir;
            joint_track.enddir = tms_trk.enddir;

            joint_track.time = tms_trk.time; // TODO: once we have LAr time working properly this should be switched to dlp_trk.time

            joint_track.Evis = dlp_trk.Evis + tms_trk.Evis;
            // TODO: add the rest of the joint_track attributes

            possibleSPINEMatches.push_back(potential_match);
          }
        }
      }
    }
    if (possiblePandoraMatches.size() > 0) {
      std::sort(possiblePandoraMatches.begin(),possiblePandoraMatches.end(),Track_match_sorter);

      std::vector<caf::SRNDLArID> matched_pan; 
      std::vector<caf::SRTMSID> matched_tmspan; // stores LAr (Pandora) and TMS indices that have already been matched

      for (unsigned int match_idx = 0; match_idx < possiblePandoraMatches.size(); match_idx++) {
        caf::SRNDTrackAssn track_match = possiblePandoraMatches[match_idx];
        double score = track_match.matchScore;
        if (score > f_cut) {
          break;}
        caf::SRNDLArID panid = track_match.larid;
        bool seen_lar = false;
        for (auto const seen_panid : matched_pan) {
          if (seen_panid.ixn == panid.ixn && seen_panid.idx == panid.idx) {
            seen_lar = true;
            break;
          }
        }
        if (seen_lar) {
          continue;}
        caf::SRTMSID tmsid = track_match.tmsid;
        bool seen_tms = false;
        for (auto const seen_tmsid : matched_tmspan) {
          if (seen_tmsid.ixn == tmsid.ixn && seen_tmsid.idx == tmsid.idx) {
            seen_tms = true;
            break;
          }
        }
        if (seen_tms) {
          continue;}
        
        matched_tmspan.push_back(tmsid);
        matched_pan.push_back(panid);
        sr.nd.trkmatch.extrap.push_back(track_match);
        sr.nd.trkmatch.nextrap += 1;
      }
    }

    if (possibleSPINEMatches.size() > 0) {
      std::sort(possibleSPINEMatches.begin(),possibleSPINEMatches.end(),Track_match_sorter);

      std::vector<caf::SRNDLArID> matched_dlp; 
      std::vector<caf::SRTMSID> matched_tmsdlp; // stores LAr (SPINE) and TMS indices that have already been matched

      for (unsigned int match_idx = 0; match_idx < possibleSPINEMatches.size(); match_idx++) {
        caf::SRNDTrackAssn track_match = possibleSPINEMatches[match_idx];
        double score = track_match.matchScore;
        if (score > f_cut) {break;}
        caf::SRNDLArID dlpid = track_match.larid;
        bool seen_lar = false;
        for (auto const seen_dlpid : matched_dlp) {
          if (seen_dlpid.ixn == dlpid.ixn && seen_dlpid.idx == dlpid.idx) {
            seen_lar = true;
            break;
          }
        }
        if (seen_lar) {continue;}
        caf::SRTMSID tmsid = track_match.tmsid;
        bool seen_tms = false;
        for (auto const seen_tmsid : matched_tmsdlp) {
          if (seen_tmsid.ixn == tmsid.ixn && seen_tmsid.idx == tmsid.idx) {
            seen_tms = true;
            break;
          }
        }
        if (seen_tms) {continue;}

        matched_tmsdlp.push_back(tmsid);
        matched_dlp.push_back(dlpid);
        sr.nd.trkmatch.extrap.push_back(track_match);
        sr.nd.trkmatch.nextrap += 1;  
      }
    }
  }
  // todo: this is a placeholder
  std::deque<Trigger> NDLArTMSUniqueMatchRecoFiller::GetTriggers(int triggerType, bool beamOnly) const
  {
    return std::deque<Trigger>();
  }

}
