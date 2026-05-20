#include "MumbleChannelJoiner.hpp"

#include <boost/algorithm/string.hpp>
using namespace std;

mumble::MumbleChannelJoiner::MumbleChannelJoiner(string channelNameRegex) : channelNameRegex(boost::regex(channelNameRegex)),
logger(log4cpp::Category::getInstance("MumbleChannelJoiner")){
    //vector<ChannelEntry> *channels = new vector<ChannelEntry>();
}

vector<mumble::MumbleChannelJoiner::ChannelEntry> mumble::MumbleChannelJoiner::channels;

void mumble::MumbleChannelJoiner::checkChannel(string channel_name, int channel_id) {
    ChannelEntry ent;
    logger.debug("Channel %s available (%d)", channel_name.c_str(), channel_id);

    ent.name = channel_name;
    ent.id = channel_id;

    channels.push_back(ent);

    if(boost::smatch s; boost::regex_match(channel_name, s, channelNameRegex)) {
      this->channel_id = channel_id;
    }
}

void mumble::MumbleChannelJoiner::maybeJoinChannel(MumbleCommunicator *mc) {
	if(channel_id > -1) {
		mc->joinChannel(channel_id);
	}
}

/* This is a secondary channel-switching object that relys on updates to the
 * class variable 'channels' for the channel list from the server.
 */
void mumble::MumbleChannelJoiner::findJoinChannel(MumbleCommunicator *mc) {
    boost::smatch s;

    int found = -1;

    for(auto &[id, name] : channels) {
        if(boost::regex_match(name, s, channelNameRegex)) {
            found = id;
        }
    }

	if(found > -1) {
		mc->joinChannel(found);
	}
}

void mumble::MumbleChannelJoiner::joinOtherChannel(MumbleCommunicator *mc, string channelNameRegex) {
    this->channelNameRegex = boost::regex(channelNameRegex);
    findJoinChannel(mc);
}


