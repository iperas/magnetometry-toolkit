﻿#include "Runner.h"
#include <common/NotImplementedException.h>
#include "RunnerCommands.h"
#include <common/Logger.h>

core::Runner::Runner(RunnerConfig config) : _config(config)
{
    _actionHandler = std::make_shared<RunnerActionHandler>();
    _webServer = std::make_shared<WebServer>();
    _webServer->port(config.webServerPort);
    _webServer->addActionHandler(_actionHandler);
}

void core::Runner::executeRunCommand(core::EbDevice::SharedPtr_t& device, RunnerCommand::SharedPtr_t& cmd, RunnerStatus::SharedPtr_t status)
{
    sLogger.Info(QString("Preparing command RUN..."));
    auto runCmd = std::static_pointer_cast<RunRunnerCommand>(cmd);
    status->isRunning = true;
    status->updated = QDateTime::currentDateTimeUtc();
    int runningIntervalMs = runCmd->intervalMilliseconds();
    int32_t actualIntervalVal;
    if (runningIntervalMs > 1000)
    {
        actualIntervalVal = runningIntervalMs / 1000;
        if (actualIntervalVal > 86400)
        {
            actualIntervalVal = 86400;
        }
    }
    else
    {
        // 5 4 3 2 Hz = 200ms, 250ms, 334ms, 500ms
        if (runningIntervalMs <= 200)
        {
            actualIntervalVal = -5;
        }
        else if (runningIntervalMs <= 250)
        {
            actualIntervalVal = -4;
        }
        else if (runningIntervalMs <= 334)
        {
            actualIntervalVal = -3;
        }
        else if (runningIntervalMs <= 500)
        {
            actualIntervalVal = -2;
        }
        else
        {
            actualIntervalVal = -1;
        }
    }
    sLogger.Info(QString("Executing command RUN with { intervalMilliseconds: %1 (converted to %2) }...")
        .arg(runningIntervalMs).arg(actualIntervalVal));
    device->sendAuto(actualIntervalVal);
    sLogger.Info(QString("Executed."));
}

void core::Runner::executeStopCommand(core::EbDevice::SharedPtr_t& device, RunnerCommand::SharedPtr_t& cmd, RunnerStatus::SharedPtr_t status)
{
    sLogger.Info(QString("Preparing command STOP..."));
    status->isRunning = false;
    status->updated = QDateTime::currentDateTimeUtc();
    sLogger.Info(QString("Executing command STOP..."));
    device->sendEnq();
    device->readEnq();
    sLogger.Info(QString("Executed."));
}

void core::Runner::executeUpdateStatus(core::EbDevice::SharedPtr_t& device, RunnerCommand::SharedPtr_t& cmd, RunnerStatus::SharedPtr_t status)
{
    sLogger.Info(QString("Executing command UPDATE-STATUS..."));
    
    // enq
    device->sendEnq();
    status->enq = device->readEnq();
    // about
    device->sendAbout();
    status->about = device->readAbout();
    // range
    device->sendGetRange();
    status->range = device->readGetRange();
    // time
    device->sendGetTime();
    status->time = device->readGetTime();
    // timeUpdated
    status->timeUpdated = QDateTime::currentDateTimeUtc();
    // updated
    status->updated = status->timeUpdated;

    sLogger.Info(QString("Executed."));
}

void core::Runner::executeSetTime(core::EbDevice::SharedPtr_t& device, RunnerCommand::SharedPtr_t& cmd, RunnerStatus::SharedPtr_t status)
{
    sLogger.Info(QString("Preparing command SET-TIME..."));
    auto timeCmd = std::static_pointer_cast<SetTimeRunnerCommand>(cmd);
    auto time = timeCmd->time();
    sLogger.Info(QString("Executing command SET-TIME..."));
    device->sendSetTime(time);
    device->readSetTime();
    device->sendGetTime();
    status->time = device->readGetTime();
    status->timeUpdated = QDateTime::currentDateTimeUtc();
    status->updated = status->timeUpdated;
    sLogger.Info(QString("Executed."));
}

void core::Runner::executeSetRange(core::EbDevice::SharedPtr_t& device, RunnerCommand::SharedPtr_t& cmd, RunnerStatus::SharedPtr_t status)
{
    sLogger.Info(QString("Preparing command SET-RANGE..."));
    auto rangeCmd = std::static_pointer_cast<SetRangeRunnerCommand>(cmd);
    auto center = rangeCmd->center();
    sLogger.Info(QString("Executing command SET-RANGE..."));
    device->sendSetRange(center);
    status->range = device->readSetRange();
    status->updated = QDateTime::currentDateTimeUtc();
    sLogger.Info(QString("Executed."));
}

void core::Runner::executeSetStandBy(core::EbDevice::SharedPtr_t& device, RunnerCommand::SharedPtr_t& cmd, RunnerStatus::SharedPtr_t status)
{
    sLogger.Info(QString("Preparing command SET-STAND-BY..."));
    auto standByCmd = std::static_pointer_cast<SetStandByRunnerCommand>(cmd);
    auto standBy = standByCmd->standBy();
    sLogger.Info(QString("Executing command SET-STAND-BY..."));
    device->sendStandBy(standBy);
    status->standBy = device->readStandBy();
    status->updated = QDateTime::currentDateTimeUtc();
    sLogger.Info(QString("Executed."));
}

void core::Runner::run()
{
    // Running commands aggregator in background thread
    _webServer->runAsync();
    
    // Creating a device
    auto device = std::make_shared<core::EbDevice>();
    device->connect(_config.devicePortName);
    device->runDiagnosticSequence();
    device->runTestAutoSequence();

    // We always do status update on start
    bool isRunning;
    {
        QMutexLocker lock(_actionHandler->dataMutex());
        RunnerCommand::SharedPtr_t updateCmd = std::make_shared<UpdateStatusRunnerCommand>();
        executeUpdateStatus(device, updateCmd, _actionHandler->status());
        isRunning = _actionHandler->status()->isRunning;
    }

    // Main worker loop
    while (true)
    {
        if (isRunning)
        {
            // receive and handle data
            QThread::sleep(1);
            // auto sample = readSample((intervalBetweenSamples + 1) * 1000);
            /*auto isValid = validateSample(sample);
            sLogger.Info(QString("Got sample #%7: field: %1, time: %2.%3, state: 0x%4, qmc: %5, isValid: %6")
                .arg(sample.field).arg(sample.time.toString(Qt::ISODate)).arg(sample.time.toMSecsSinceEpoch() % 1000)
                .arg(sample.state, 2, 16).arg(sample.qmc).arg(isValid).arg(i + 1));
            assertTrue(sample.state != FatalError, "Errors if any are not fatal.");*/
        }
        else
        {
            // sleep 1 second as a fallback for data logging
            QThread::sleep(1);
        }

        // run commands
        sLogger.Debug("Reading pending commands...");
        QMutexLocker lock(_actionHandler->dataMutex());
        while (!_actionHandler->commands().empty())
        {
            sLogger.Debug("Found a command...");
            auto cmd = _actionHandler->commands().dequeue();
            switch (cmd->type())
            {
            case Run:
                executeRunCommand(device, cmd, _actionHandler->status());
                isRunning = _actionHandler->status()->isRunning;
                break;
            case Stop:
                executeStopCommand(device, cmd, _actionHandler->status());
                isRunning = _actionHandler->status()->isRunning;
                break;
            case UpdateStatus:
                executeUpdateStatus(device, cmd, _actionHandler->status());
                break;
            case SetTime:
                executeSetTime(device, cmd, _actionHandler->status());
                break;
            case SetRange:
                executeSetRange(device, cmd, _actionHandler->status());
                break;
            case SetStandBy:
                executeSetStandBy(device, cmd, _actionHandler->status());
                break;
            default:
                throw Common::NotImplementedException();
            }
            sLogger.Debug("Done reading command.");
        }
    }
}