#pragma once

#include <boost/noncopyable.hpp>
#include <log4cpp/Category.hh>

#include <vector>
#include <string>
#include <boost/regex.hpp>
#include "MumbleCommunicator.hpp"

namespace mumble {
    class MumbleChannelJoiner : boost::noncopyable {

        struct ChannelEntry {
            int id;
            std::string name;
        };

    public:
        MumbleChannelJoiner(std::string channelNameRegex);

        void checkChannel(std::string channel_name, int channel_id);
        void maybeJoinChannel(MumbleCommunicator *mc);
        void findJoinChannel(MumbleCommunicator *mc);
        void joinOtherChannel(MumbleCommunicator *mc, std::string channelNameRegex);

    private:
        log4cpp::Category &logger;
        boost::regex channelNameRegex;
        int channel_id;
        static std::vector<ChannelEntry> channels;
    };
}
