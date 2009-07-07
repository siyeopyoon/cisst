/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: mtsDeviceInterfaceProxyClient.cpp 145 2009-03-18 23:32:40Z mjung5 $

  Author(s):  Min Yang Jung
  Created on: 2009-04-24

  (C) Copyright 2009 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstOSAbstraction/osaSleep.h>
#include <cisstMultiTask/mtsDeviceInterfaceProxyClient.h>
#include <cisstMultiTask/mtsTaskManager.h>
#include <cisstMultiTask/mtsCommandVoidProxy.h>
#include <cisstMultiTask/mtsCommandWriteProxy.h>
#include <cisstMultiTask/mtsCommandReadProxy.h>
#include <cisstMultiTask/mtsCommandQualifiedReadProxy.h>
#include <cisstMultiTask/mtsRequiredInterface.h>
#include <cisstMultiTask/mtsDeviceProxy.h>

CMN_IMPLEMENT_SERVICES(mtsDeviceInterfaceProxyClient);

#define DeviceInterfaceProxyClientLogger(_log) Logger->trace("mtsDeviceInterfaceProxyClient", _log)
#define DeviceInterfaceProxyClientLoggerError(_log1, _log2) \
    {   std::stringstream s;\
        s << "mtsDeviceInterfaceProxyClient: " << _log1 << _log2;\
        BaseType::Logger->error(s.str());  }

mtsDeviceInterfaceProxyClient::mtsDeviceInterfaceProxyClient(
    const std::string & propertyFileName, const std::string & propertyName) :
        BaseType(propertyFileName, propertyName)
{
}

mtsDeviceInterfaceProxyClient::~mtsDeviceInterfaceProxyClient()
{
}

void mtsDeviceInterfaceProxyClient::Start(mtsTask * callingTask)
{
    // Initialize Ice object.
    // Notice that a worker thread is not created right now.
    Init();
    
    if (InitSuccessFlag) {
        // Client configuration for bidirectional communication
        Ice::ObjectAdapterPtr adapter = IceCommunicator->createObjectAdapter("");
        Ice::Identity ident;
        ident.name = GetGUID();
        ident.category = "";

        mtsDeviceInterfaceProxy::DeviceInterfaceClientPtr client = 
            new DeviceInterfaceClientI(IceCommunicator, Logger, DeviceInterfaceServerProxy, this);
        adapter->add(client, ident);
        adapter->activate();
        DeviceInterfaceServerProxy->ice_getConnection()->setAdapter(adapter);
        DeviceInterfaceServerProxy->AddClient(ident);

        // Create a worker thread here and returns immediately.
        ThreadArgumentsInfo.argument = callingTask;
        ThreadArgumentsInfo.proxy = this;        
        ThreadArgumentsInfo.Runner = mtsDeviceInterfaceProxyClient::Runner;

        WorkerThread.Create<ProxyWorker<mtsTask>, ThreadArguments<mtsTask>*>(
            &ProxyWorkerInfo, &ProxyWorker<mtsTask>::Run, &ThreadArgumentsInfo, "S-PRX");
    }
}

void mtsDeviceInterfaceProxyClient::StartClient()
{
    Sender->Start();

    // This is a blocking call that should run in a different thread.
    IceCommunicator->waitForShutdown();
}

void mtsDeviceInterfaceProxyClient::Runner(ThreadArguments<mtsTask> * arguments)
{
    mtsDeviceInterfaceProxyClient * ProxyClient = 
        dynamic_cast<mtsDeviceInterfaceProxyClient*>(arguments->proxy);

    ProxyClient->GetLogger()->trace("mtsDeviceInterfaceProxyClient", "Proxy client starts.");

    try {
        ProxyClient->StartClient();        
    } catch (const Ice::Exception& e) {
        ProxyClient->GetLogger()->trace("mtsDeviceInterfaceProxyClient exception: ", e.what());
    } catch (const char * msg) {
        ProxyClient->GetLogger()->trace("mtsDeviceInterfaceProxyClient exception: ", msg);
    }

    ProxyClient->OnThreadEnd();
}

void mtsDeviceInterfaceProxyClient::Stop()
{
    OnThreadEnd();
}

void mtsDeviceInterfaceProxyClient::OnThreadEnd()
{
    DeviceInterfaceProxyClientLogger("Proxy client ends.");

    BaseType::OnThreadEnd();

    Sender->Stop();
}

//-------------------------------------------------------------------------
//  Methods to Receive and Process Events (Server -> Client)
//-------------------------------------------------------------------------
bool mtsDeviceInterfaceProxyClient::ReceiveGetListsOfEventGeneratorsRegistered(
    const std::string & serverTaskProxyName, 
    const std::string & requiredInterfaceName,
    mtsDeviceInterfaceProxy::ListsOfEventGeneratorsRegistered & eventGeneratorProxies) const
{
    mtsTaskManager * taskManager = mtsTaskManager::GetInstance();

    mtsDeviceProxy * serverTaskProxy = dynamic_cast<mtsDeviceProxy*>(
        taskManager->GetDevice(serverTaskProxyName));
    CMN_ASSERT(serverTaskProxy);

    return serverTaskProxy->GetEventGeneratorProxyPointers(
        requiredInterfaceName, eventGeneratorProxies);
}

void mtsDeviceInterfaceProxyClient::ReceiveExecuteEventVoid(const int commandId)
{
    mtsFunctionVoid * functionVoid = reinterpret_cast<mtsFunctionVoid *>(commandId);
    CMN_ASSERT(functionVoid);

    (*functionVoid)();
}

void mtsDeviceInterfaceProxyClient::ReceiveExecuteEventWriteSerialized(
    const int commandId, const std::string argument)
{
    mtsFunctionWrite * functionWrite = reinterpret_cast<mtsFunctionWrite*>(commandId);
    CMN_ASSERT(functionWrite);

    static char buf[1024];
    sprintf(buf, "ReceiveExecuteEventWriteSerialized: %d bytes received", argument.size());
    Logger->trace("TIClient", buf);

    // Deserialization
    DeSerializationBuffer.str("");
    DeSerializationBuffer << argument;
    
    const mtsGenericObject * obj = dynamic_cast<mtsGenericObject *>(DeSerializer->DeSerialize());
    CMN_ASSERT(obj);
    (*functionWrite)(*obj);
}

//-------------------------------------------------------------------------
//  Methods to Send Events
//-------------------------------------------------------------------------
const bool mtsDeviceInterfaceProxyClient::SendGetProvidedInterfaceInfo(
    const std::string & providedInterfaceName,
    mtsDeviceInterfaceProxy::ProvidedInterfaceInfo & providedInterfaceInfo)
{
    Logger->trace("TIClient", ">>>>> SEND: SendGetProvidedInterface");

    return DeviceInterfaceServerProxy->GetProvidedInterfaceInfo(
        providedInterfaceName, providedInterfaceInfo);
}

bool mtsDeviceInterfaceProxyClient::SendConnectServerSide(
    const std::string & userTaskName, const std::string & requiredInterfaceName,
    const std::string & resourceTaskName, const std::string & providedInterfaceName)
{
    Logger->trace("TIClient", ">>>>> SEND: SendConnectServerSide");

    return DeviceInterfaceServerProxy->ConnectServerSide(
        userTaskName, requiredInterfaceName, resourceTaskName, providedInterfaceName);
}

void mtsDeviceInterfaceProxyClient::SendGetCommandId(
    const std::string & clientTaskProxyName,
    mtsDeviceInterfaceProxy::FunctionProxySet & functionProxies)
{
    Logger->trace("TIClient", ">>>>> SEND: SendGetCommandId");

    DeviceInterfaceServerProxy->GetCommandId(clientTaskProxyName, functionProxies);
}

void mtsDeviceInterfaceProxyClient::SendExecuteCommandVoid(const int commandId) const
{
    //Logger->trace("TIClient", ">>>>> SEND: SendExecuteCommandVoid");

    DeviceInterfaceServerProxy->ExecuteCommandVoid(commandId);
}

void mtsDeviceInterfaceProxyClient::SendExecuteCommandWriteSerialized(
    const int commandId, const cmnGenericObject & argument)
{
    //Logger->trace("TIClient", ">>>>> SEND: SendExecuteCommandQualifiedRead");

    // Serialization
    std::string serializedData;
    Serialize(argument, serializedData);
    
    DeviceInterfaceServerProxy->ExecuteCommandWriteSerialized(commandId, serializedData);
}

void mtsDeviceInterfaceProxyClient::SendExecuteCommandReadSerialized(
    const int commandId, cmnGenericObject & argument)
{
    //Logger->trace("TIClient", ">>>>> SEND: SendExecuteCommandReadSerialized");

    std::string serializedData;

    DeviceInterfaceServerProxy->ExecuteCommandReadSerialized(commandId, serializedData);

    // Deserialization
    DeSerializationBuffer.str("");
    DeSerializationBuffer << serializedData;
    DeSerializer->DeSerialize(argument);
}

void mtsDeviceInterfaceProxyClient::SendExecuteCommandQualifiedReadSerialized(
    const int commandId, const cmnGenericObject & argument1, cmnGenericObject & argument2)
{
    ////Logger->trace("TIClient", ">>>>> SEND: SendExecuteCommandQualifiedRead");
    //
    //double value = argument1.Data;
    //double outValue = 0.0;

    //DeviceInterfaceServerProxy->ExecuteCommandQualifiedRead(commandId, value, outValue);

    //cmnDouble out(outValue);
    //argument2 = out;
}

//-------------------------------------------------------------------------
//  Send Methods
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//  Definition by mtsDeviceInterfaceProxy.ice
//-------------------------------------------------------------------------
mtsDeviceInterfaceProxyClient::DeviceInterfaceClientI::DeviceInterfaceClientI(
    const Ice::CommunicatorPtr& communicator,                           
    const Ice::LoggerPtr& logger,
    const mtsDeviceInterfaceProxy::DeviceInterfaceServerPrx& server,
    mtsDeviceInterfaceProxyClient * DeviceInterfaceClient)
    : Runnable(true), 
      Communicator(communicator), Logger(logger),
      Server(server), DeviceInterfaceClient(DeviceInterfaceClient),      
      Sender(new SendThread<DeviceInterfaceClientIPtr>(this))      
{
}

void mtsDeviceInterfaceProxyClient::DeviceInterfaceClientI::Start()
{
    DeviceInterfaceProxyClientLogger("Send thread starts");

    Sender->start();
}

void mtsDeviceInterfaceProxyClient::DeviceInterfaceClientI::Run()
{
    bool flag = true;

    while(true)
    {
        if (flag) {
            // Send a set of task names
            /*
            mtsDeviceInterfaceProxy::TaskList localTaskList;
            std::vector<std::string> myTaskNames;
            mtsTaskManager::GetInstance()->GetNamesOfTasks(myTaskNames);

            localTaskList.taskNames.insert(
                localTaskList.taskNames.end(),
                myTaskNames.begin(),
                myTaskNames.end());

            localTaskList.taskManagerID = TaskManagerClient->GetGUID();

            Server->AddTaskManager(localTaskList);
            */

            flag = false;
        }

        timedWait(IceUtil::Time::milliSeconds(1));
    }
}

void mtsDeviceInterfaceProxyClient::DeviceInterfaceClientI::Stop()
{
    DeviceInterfaceProxyClientLogger("Send thread is terminating.");

    IceUtil::ThreadPtr callbackSenderThread;
    {
        //IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);

        DeviceInterfaceProxyClientLogger("Destroying sender.");
        Runnable = false;

        notify();

        callbackSenderThread = Sender;
        Sender = 0; // Resolve cyclic dependency.
    }
    callbackSenderThread->getThreadControl().join();
}

//-----------------------------------------------------------------------------
//  Device Interface Proxy Client Implementation
//-----------------------------------------------------------------------------
bool mtsDeviceInterfaceProxyClient::DeviceInterfaceClientI::GetListsOfEventGeneratorsRegistered(
    const std::string & serverTaskProxyName, 
    const std::string & requiredInterfaceName,
    mtsDeviceInterfaceProxy::ListsOfEventGeneratorsRegistered & eventGeneratorProxies, 
    const ::Ice::Current&) const
{
    Logger->trace("TIClient", "<<<<< RECV: GetListsOfEventGeneratorsRegistered");

    return DeviceInterfaceClient->ReceiveGetListsOfEventGeneratorsRegistered(
        serverTaskProxyName, requiredInterfaceName, eventGeneratorProxies);
}

void mtsDeviceInterfaceProxyClient::DeviceInterfaceClientI::ExecuteEventVoid(
    ::Ice::Int commandId, const ::Ice::Current&)
{
    Logger->trace("TIClient", "<<<<< RECV: ExecuteEventVoid");

    DeviceInterfaceClient->ReceiveExecuteEventVoid(commandId);
}

void mtsDeviceInterfaceProxyClient::DeviceInterfaceClientI::ExecuteEventWriteSerialized(
    ::Ice::Int commandId, const ::std::string& argument, const ::Ice::Current&)
{
    Logger->trace("TIClient", "<<<<< RECV: ExecuteEventWriteSerialized");

    DeviceInterfaceClient->ReceiveExecuteEventWriteSerialized(commandId, argument);
}