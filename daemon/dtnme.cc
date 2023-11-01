/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*
 *    Modifications made to this file by the patch file dtn2_mfs-33289-1.patch
 *    are Copyright 2015 United States Government as represented by NASA
 *       Marshall Space Flight Center. All Rights Reserved.
 *
 *    Released under the NASA Open Source Software Agreement version 1.3;
 *    You may obtain a copy of the Agreement at:
 * 
 *        http://ti.arc.nasa.gov/opensource/nosa/
 * 
 *    The subject software is provided "AS IS" WITHOUT ANY WARRANTY of any kind,
 *    either expressed, implied or statutory and this agreement does not,
 *    in any manner, constitute an endorsement by government agency of any
 *    results, designs or products resulting from use of the subject software.
 *    See the Agreement for the specific language governing permissions and
 *    limitations.
 */

#ifdef HAVE_CONFIG_H
#  include <dtn-config.h>
#endif

#include <malloc.h>

#include <errno.h>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <third_party/oasys/debug/Log.h>
#include <third_party/oasys/io/NetUtils.h>
#include <third_party/oasys/tclcmd/ConsoleCommand.h>
#include <third_party/oasys/tclcmd/TclCommand.h>
#include <third_party/oasys/thread/Timer.h>
#include <third_party/oasys/util/App.h>
#include <third_party/oasys/util/Getopt.h>
#include <third_party/oasys/util/StringBuffer.h>

#include "applib/APIServer.h"
#include "cmd/TestCommand.h"
#include "servlib/DTNServer.h"
#include "storage/DTNStorageConfig.h"
#include "bundling/BundleDaemon.h"

extern const char* dtn_version;

/**
 * Namespace for the dtn daemon source code.
 */
namespace dtn {

/**
 * Thin class that implements the daemon itself.
 */
class DTNME : public oasys::App {
public:
    DTNME();
    int main(int argc, char* argv[]);

protected:
    TestCommand*          testcmd_;
    oasys::ConsoleCommand* consolecmd_;
    DTNStorageConfig      storage_config_;
    
    // virtual from oasys::App
    void fill_options();
    
    void output_welcome_message();
    void init_testcmd(int argc, char* argv[]);
    void run_console();
};

//----------------------------------------------------------------------
DTNME::DTNME()
    : App("dtnme", "dtnme", dtn_version),
      testcmd_(NULL),
      consolecmd_(NULL),
      storage_config_("storage",			// command name
                      "berkeleydb",			// storage type
                      "DTN",				// DB name
                      INSTALL_LOCALSTATEDIR "/dtn/db")	// DB directory
{
    //umask(0002); // set deafult mask to prevent world write/delete

    // override default logging settings
    loglevel_ = oasys::LOG_NOTICE;
    debugpath_ = "~/.dtnmedebug";
    
    // override defaults from oasys storage config
    storage_config_.db_max_tx_ = 1000;

    testcmd_    = new TestCommand();
    consolecmd_ = new oasys::ConsoleCommand("dtnme% ");
}

//----------------------------------------------------------------------
void
DTNME::fill_options()
{
    fill_default_options(DAEMONIZE_OPT | CONF_FILE_OPT);
    
    opts_.addopt(
        new oasys::BoolOpt('t', "tidy", &storage_config_.tidy_,
                           "clear database and initialize tables on startup"));

    opts_.addopt(
        new oasys::BoolOpt(0, "init-db", &storage_config_.init_,
                           "initialize database on startup"));

    opts_.addopt(
        new oasys::InAddrOpt(0, "console-addr", &consolecmd_->addr_, "<addr>",
                             "set the console listening addr (default off)"));
    
    opts_.addopt(
        new oasys::UInt16Opt(0, "console-port", &consolecmd_->port_, "<port>",
                             "set the console listening port (default off)"));
    
    opts_.addopt(
        new oasys::IntOpt('i', 0, &testcmd_->id_, "<id>",
                          "set the test id"));
}

//----------------------------------------------------------------------
void
DTNME::init_testcmd(int argc, char* argv[])
{
    for (int i = 0; i < argc; ++i) {
        testcmd_->argv_.append(argv[i]);
        testcmd_->argv_.append(" ");
    }

    testcmd_->bind_vars();
    oasys::TclCommandInterp::instance()->reg(testcmd_);
}

//----------------------------------------------------------------------
void
DTNME::run_console()
{
    // launch the console server
    if (consolecmd_->port_ != 0) {
        log_info("starting console on %s:%d",
                   intoa(consolecmd_->addr_), consolecmd_->port_);
        
        oasys::TclCommandInterp::instance()->
            command_server(consolecmd_->prompt_.c_str(),
                           consolecmd_->addr_, consolecmd_->port_);
    }
    
    if (daemonize_ || (consolecmd_->stdio_ == false)) {
        oasys::TclCommandInterp::instance()->event_loop();
    } else {
        oasys::TclCommandInterp::instance()->
            command_loop(consolecmd_->prompt_.c_str());
    }
}

//----------------------------------------------------------------------
void
DTNME::output_welcome_message()
{
    std::string msg1, msg2, msg3, msg4, msg5, msg6;

    msg1 = "=====================  NOTICE  =====================";
    msg2 = "     DTNME supports both BPv6 and BPv7 bundles.";
                                
    if (BundleDaemon::params_.api_send_bp_version7_) {
        msg3 = "     By default, apps will generate BPv7 bundles";
        msg4 = "     Use the -V6 option to generate BPv6 bundles";
        msg5 = "     or change the default with the command:";
        msg6 = "           param set api_send_bp7 false";
    } else {
        msg3 = "     By default, apps will generate BPv6 bundles";
        msg4 = "     Use the -V7 option to generate BPv7 bundles";
        msg5 = "     or change the default with the command:";
        msg6 = "           param set api_send_bp7 true";
    }


    // output to the log file
    log_always(msg1.c_str());
    log_always(msg2.c_str());
    log_always(msg3.c_str());
    log_always(msg4.c_str());
    log_always(msg5.c_str());
    log_always(msg6.c_str());
    log_always(msg1.c_str());

    // output to stdout
    printf("\n\n%s\n", msg1.c_str());
    printf("%s\n\n", msg2.c_str());
    printf("%s\n", msg3.c_str());
    printf("%s\n", msg4.c_str());
    printf("%s\n", msg5.c_str());
    printf("%s\n", msg6.c_str());
    printf("%s\n\n", msg1.c_str());

}

//----------------------------------------------------------------------
int
DTNME::main(int argc, char* argv[])
{
    init_app(argc, argv);

    log_notice("DTN daemon starting up... (pid %d)", getpid());


    if (oasys::TclCommandInterp::init(argv[0], "/dtn/tclcmd") != 0)
    {
        log_crit("Can't init TCL");
        notify_and_exit(1);
    }

    // stop thread creation b/c of initialization dependencies
    oasys::Thread::activate_start_barrier();

    DTNServer* dtnserver = new DTNServer("/dtnme", &storage_config_);
    APIServer* apiserver = new APIServer();

    dtnserver->init();

    oasys::TclCommandInterp::instance()->reg(consolecmd_);
    init_testcmd(argc, argv);

    if (! dtnserver->parse_conf_file(conf_file_, conf_file_set_)) {
        log_err("error in configuration file, exiting...");
        notify_and_exit(1);
    }

    // configure the umask for controlling file permissions
    // - the mask is inverted when applied so zero bits allow the particular access
    uint16_t mask = ~BundleDaemon::params_.file_permissions_;
    // - allow owner full access regardless of what the user specified
    //mask &= 0x077; 
    umask(mask); // set deafult mask to prevent world write

    log_info("file permissions: 0%3.3o", (~mask & 0777));

    output_welcome_message();

    if (storage_config_.init_)
    {
        log_notice("initializing persistent data store");
    }

    if (! dtnserver->init_datastore()) {
        log_err("error initializing data store, exiting...");
        notify_and_exit(1);
    }
    
    // If we're running as --init-db, make an empty database and exit
    if (storage_config_.init_ && !storage_config_.tidy_)
    {
        dtnserver->close_datastore();
        log_info("database initialization complete.");
        notify_and_exit(0);
    }
    
    if (BundleDaemon::instance()->local_eid() == BD_NULL_EID())
    {
        log_err("no local eid specified; use the 'route local_eid' command");
        notify_and_exit(1);
    }

    // if we've daemonized, now is the time to notify our parent
    // process that we've successfully initialized
    if (daemonize_) {
        daemonizer_.notify_parent(0);
    }
    
    log_notice("starting APIServer on %s:%d", 
                 intoa(apiserver->local_addr()), apiserver->local_port());

    // set console info before starting the dtnserver (BundleDaemon)
    dtnserver->set_console_info(consolecmd_->addr_, consolecmd_->port_, consolecmd_->stdio_);

    dtnserver->start();
    if (apiserver->enabled()) {
        apiserver->bind_listen_start(apiserver->local_addr(), 
                                     apiserver->local_port());
    }
    oasys::Thread::release_start_barrier(); // run blocked threads

    // if the test script specified something to run for the test,
    // then execute it now
    if (testcmd_->initscript_.length() != 0) {
        oasys::TclCommandInterp::instance()->
            exec_command(testcmd_->initscript_.c_str());
    }

    // allow startup messages to be flushed to standard-out before
    // the prompt is displayed
    oasys::Thread::yield();
    
    run_console();

    log_notice("command loop exited... shutting down daemon");

    apiserver->stop();
    apiserver->shutdown_hook();
    
    oasys::TclCommandInterp::shutdown();
    dtnserver->shutdown();
    
    // close out servers
    delete dtnserver;
    // don't delete apiserver; keep it around so slow APIClients can finish
    
    delete apiserver;


    // give other threads (like logging) a moment to catch up before exit
//    oasys::Thread::yield();
//dzdebug    sleep(3);
    
    // kill logging
    //dzdebug oasys::Log::shutdown();
    
    while (! BundleDaemon::instance()->is_stopped()) {
        oasys::Thread::yield();
    }

    BundleDaemon::reset();


    return 0;
}

} // namespace dtn

int
main(int argc, char* argv[])
{
    dtn::DTNME dtnme;

    //dzdebug
    int status = mallopt(M_ARENA_MAX, 48);
    if (status == 0) {
        printf("error setting M_ARENA_MAX to 48\n");
    } else {
        //printf("success setting M_ARENA_MAX to 48\n");
    }

    int val = 1024*1024*4*8;
    status = mallopt(M_MMAP_THRESHOLD, val);
    if (status == 0) {
        printf("error setting M_MMAP_THRESHOLD to %d\n", val);
    } else {
        //printf("success setting M_MMAP_THRESHOLD to %d\n", val);
    }


    dtnme.main(argc, argv);
}
