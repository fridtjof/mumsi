#include "PjsuaCommunicator.hpp"
#include "MumbleCommunicator.hpp"
#include "IncomingConnectionValidator.hpp"
#include "MumbleChannelJoiner.hpp"
#include "Configuration.hpp"

#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/PatternLayout.hh>

#include <memory>

#ifdef USE_DEBUG
#include <execinfo.h>
#endif

#include "main.hpp"

namespace {

std::weak_ptr<boost::asio::io_context> g_ioService;

/*
 * Code from http://stackoverflow.com/a/77336/5419223
 */
void sigsegv_handler(int sig) {
    constexpr int STACK_DEPTH = 10;
    void *array[STACK_DEPTH];

    #ifdef USE_DEBUG
    const auto size = backtrace(array, STACK_DEPTH);
    #endif

    fprintf(stderr, "ERROR: signal %d:\n", sig);
    #ifdef USE_DEBUG
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    #endif
    exit(1);
}

void sigint_handler(int sig)
{
    fprintf(stderr, "Caught SIGINT, trying to stop\n");
    if (const auto io = g_ioService.lock()) {
        io->stop();
    }
}

void SetupSignalHandlers()
{
    struct sigaction sigIntHandler{};

    sigIntHandler.sa_handler = sigsegv_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGSEGV, &sigIntHandler, nullptr);

    memset(&sigIntHandler, 0, sizeof(sigIntHandler));
    sigIntHandler.sa_handler = sigint_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, nullptr);
}

} // anonymous namespace

int main(int argc, char *argv[]) {
    SetupSignalHandlers();
    int max_calls;

    // ReSharper disable once CppDFAMemoryLeak deleted by logger
    log4cpp::Appender* appender = new log4cpp::OstreamAppender("console", &std::cout);
    // ReSharper disable once CppDFAMemoryLeak deleted by appender
    auto layout = new log4cpp::PatternLayout;
    layout->setConversionPattern("%d [%p] %c: %m%n");
    appender->setLayout(layout);
    log4cpp::Category &logger = log4cpp::Category::getRoot();
    logger.setPriority(log4cpp::Priority::DEBUG);
    //logger.setPriority(log4cpp::Priority::NOTICE);
    logger.addAppender(appender);

    if (argc == 1) {
        logger.crit("No configuration file provided. Use %s {config file}", argv[0]);
        std::exit(1);
    }

    config::Configuration conf(argv[1]);

    logger.setPriority(log4cpp::Priority::getPriorityValue(conf.getString("general.logLevel")));

    sip::IncomingConnectionValidator connectionValidator(conf.getString("sip.validUriExpression"));

    auto ioService = std::make_shared<boost::asio::io_context>();
    g_ioService = ioService;

    try {
        max_calls = conf.getInt("sip.max_calls");
    } catch (...) {
        max_calls = 1;
    }

    sip::PjsuaCommunicator pjsuaCommunicator(connectionValidator, conf.getInt("sip.frameLength"), max_calls);

    try {
        pjsuaCommunicator.pins = conf.getChildren("app");
    } catch (...) {
    }

    mumble::MumbleCommunicatorConfig mumbleConf;
    mumbleConf.host = conf.getString("mumble.host");
    mumbleConf.port = conf.getInt("mumble.port");
    mumbleConf.user = conf.getString("mumble.user");
    mumbleConf.password = conf.getString("mumble.password");
    mumbleConf.opusEncoderBitrate = conf.getInt("mumble.opusEncoderBitrate");
    /* default to 'false' if not found */
    try {
        mumbleConf.autodeaf = conf.getBool("mumble.autodeaf");
    } catch (...) {
        mumbleConf.autodeaf = false;
    }

    mumbleConf.comment = conf.getString("app.comment", "");

    pjsuaCommunicator.file_welcome = conf.getString("files.welcome", "welcome.wav");
    pjsuaCommunicator.file_prompt_pin = conf.getString("files.prompt_pin", "prompt-pin.wav");
    pjsuaCommunicator.file_entering_channel = conf.getString("files.entering_channel", "entering-channel.wav");
    pjsuaCommunicator.file_announce_new_caller = conf.getString("files.announce_new_caller", "announce-new-caller.wav");
    pjsuaCommunicator.file_invalid_pin = conf.getString("files.invalid_pin", "invalid-pin.wav");
    pjsuaCommunicator.file_goodbye = conf.getString("files.goodbye", "goodbye.wav");
    pjsuaCommunicator.file_mute_on = conf.getString("files.mute_on", "mute-on.wav");
    pjsuaCommunicator.file_mute_off = conf.getString("files.mute_off", "mute-off.wav");
    pjsuaCommunicator.file_menu = conf.getString("files.menu", "menu.wav");

    std::string defaultChan = conf.getString("mumble.channelNameExpression"); 

    mumble::MumbleChannelJoiner mumbleChannelJoiner(defaultChan);
    mumble::MumbleChannelJoiner mumbleOtherChannelJoiner(defaultChan);

    for (int i = 0; i<max_calls; i++) {
        auto *mumcom = new mumble::MumbleCommunicator(*ioService);
        mumcom->callId = i;

        using namespace std::placeholders;
        // Passing audio input from SIP to Mumble
        pjsuaCommunicator.calls[i].onIncomingPcmSamples = [mumcom](auto samples, auto length) {
            mumcom->sendPcmSamples(samples, length);
        };

        // PJ sends text message to Mumble
        pjsuaCommunicator.calls[i].onStateChange = [mumcom](const auto& message) { mumcom->sendTextMessage(message); };

        // Send mute/deaf to Mumble
        /*pjsuaCommunicator.calls[i].onMuteDeafChange = [mumcom](int val) {
            mumcom->mutedeaf(val);
        };*/

        // Send UserState to Mumble
        pjsuaCommunicator.calls[i].sendUserState = [mumcom](auto field, bool val) {
            mumcom->sendUserState(field, val);
        };

        // Send UserState to Mumble
        pjsuaCommunicator.calls[i].sendUserStateStr = [mumcom](auto field, const std::string& val) {
            mumcom->sendUserState(field, val);
        };

        // Send TextMessage to Mumble
        pjsuaCommunicator.calls[i].sendTextMessageStr = [mumcom](auto field, const auto& message) {
            mumcom->sendTextMessageStr(field, message);
        };

        // PJ triggers Mumble connect
        pjsuaCommunicator.calls[i].onConnect = [mumcom](const auto& address) { mumcom->onConnect(address); };

        // PJ triggers Mumble disconnect
        pjsuaCommunicator.calls[i].onDisconnect = [mumcom] { mumcom->onDisconnect(); };

        // PJ notifies Mumble that Caller Auth is done
        pjsuaCommunicator.calls[i].onCallerAuth = [mumcom] { mumcom->onCallerAuth(); };

        // PJ notifies Mumble that Caller Unauth is done
        //pjsuaCommunicator.calls[i].onCallerUnauth = [mumcom] { mumcom->onCallerUnauth(); };

        // PJ notifies Mumble that Caller Auth is done
        pjsuaCommunicator.calls[i].joinDefaultChannel = [&mumbleChannelJoiner, mumcom] {
            mumbleChannelJoiner.findJoinChannel(mumcom);
        };

        // PJ notifies Mumble to join other channel
        pjsuaCommunicator.calls[i].joinOtherChannel = [mumcom, &mumbleOtherChannelJoiner](const auto & channelNameRegex) {
            mumbleOtherChannelJoiner.joinOtherChannel(mumcom, channelNameRegex);
        };

        // Passing audio from Mumble to SIP
        mumcom->onIncomingPcmSamples = std::bind(
                &sip::PjsuaCommunicator::sendPcmSamples,
                &pjsuaCommunicator,
                _1, _2, _3, _4, _5);

        // Handle Channel State messages from Mumble
        mumcom->onIncomingChannelState = [&mumbleChannelJoiner](const auto&  channel_name, auto channel_id) {
            mumbleChannelJoiner.checkChannel(channel_name, channel_id);
        };

        // Handle Server Sync message from Mumble
        mumcom->onServerSync = [&mumbleChannelJoiner, mumcom] { mumbleChannelJoiner.maybeJoinChannel(mumcom); };

        if ( max_calls > 1 ) {
            mumbleConf.user = conf.getString("mumble.user") + '-' + std::to_string(i);
        }

        try {
            if ( conf.getBool("mumble.use_certs") ) {
                mumbleConf.cert_file = mumbleConf.user + "-cert.pem";
                mumbleConf.privkey_file = mumbleConf.user + "-key.pem";
            }
        } catch (...) {
            logger.info("Client certs not enabled in config");
        }
        mumcom->connect(mumbleConf);
    }

    pjsuaCommunicator.connect(
            conf.getString("sip.host"),
            conf.getString("sip.user"),
            conf.getString("sip.password"),
            conf.getInt("sip.port"));

    logger.info("Application started.");

    ioService->run();

    return 0;
}

