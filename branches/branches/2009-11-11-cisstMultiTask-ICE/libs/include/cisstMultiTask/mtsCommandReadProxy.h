/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: mtsCommandReadProxy.h 75 2009-02-24 16:47:20Z adeguet1 $

  Author(s):  Min Yang Jung
  Created on: 2009-04-29

  (C) Copyright 2009 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


/*!
  \file
  \brief Defines a command with one argument 
*/

#ifndef _mtsCommandReadProxy_h
#define _mtsCommandReadProxy_h

#include <cisstMultiTask/mtsCommandReadOrWriteBase.h>
#include <cisstMultiTask/mtsComponentInterfaceProxyClient.h>
#include <cisstMultiTask/mtsProxySerializer.h>

/*!
  \ingroup cisstMultiTask

  mtsCommandReadProxy is a proxy for mtsCommandRead. This proxy contains
  CommandId set as a function pointer of which type is mtsFunctionRead.
  When Execute() method is called, the CommandId is sent to the server task
  over networks with one payload. The provided interface proxy manages 
  this process.
*/
class mtsCommandReadProxy: public mtsCommandReadBase {
    
    friend class mtsDeviceProxy;

public:
    typedef mtsCommandReadBase BaseType;

protected:
    /*! CommandId is set as a pointer to a mtsFunctionRead at peer's
        memory space which binds to an actual write command. */
    CommandIDType CommandId;

    /*! Argument prototype. Deserialization recovers the original argument
        prototype object. */
    //mtsGenericObject * ArgumentPrototype;

    /*! Device interface proxy objects which execute a command at 
        peer's memory space across networks. */
    /*! Initialization method */
    void Initialize()
    {
        this->ArgumentPrototype = 0;
    }

    /*! Device interface proxy objects which execute a read command at 
        peer's memory space across networks. */
    //mtsComponentInterfaceProxyClient * ProvidedInterfaceProxy;

    /*! Per-command serializer and deserializer */
    mtsProxySerializer Serializer;

public:
    mtsCommandReadProxy(const CommandIDType commandId, 
                        mtsComponentInterfaceProxyClient * providedInterfaceProxy):
        mtsCommandReadBase(),
        CommandId(commandId)
//        ProvidedInterfaceProxy(providedInterfaceProxy)
    {
        Initialize();
    }

    mtsCommandReadProxy(const CommandIDType commandId,
                        mtsComponentInterfaceProxyClient * providedInterfaceProxy,
                        const std::string & name):
        mtsCommandReadBase(name),
        CommandId(commandId)
        //ProvidedInterfaceProxy(providedInterfaceProxy)
    {
        Initialize();
    }

    virtual ~mtsCommandReadProxy() {
        if (this->ArgumentPrototype) {
            delete this->ArgumentPrototype;
        }
    }
    

    /*! Update CommandId. */
    void SetCommandId(const CommandIDType & newCommandId) {
        CommandId = newCommandId;

        //ProvidedInterfaceProxy->AddPerCommandSerializer(CommandId, &Serializer);

        // MJUNG: Currently, there are only two types of events: eventVoid, eventWrite.
        // Thus, we don't need to modify the mtsCommandReadProxy or 
        // the mtsCommandQualifiedReadProxy class such that it can support events.
        // In the future, however, if the design is extended such that event types such 
        // as eventRead or eventQualifiedRead are introduced, we should update here as
        // well.
    }

public:
    /*! The execute method. */
    virtual mtsCommandBase::ReturnType Execute(mtsGenericObject & argument) {
        if (this->IsEnabled()) {
            //CMN_ASSERT(ProvidedInterfaceProxy);
            //ProvidedInterfaceProxy->SendExecuteCommandReadSerialized(CommandId, argument);
            return mtsCommandBase::DEV_OK;
        }
        return mtsCommandBase::DISABLED;
    }
    
    /*! For debugging. Generate a human readable output for the
        command object */
    void ToStream(std::ostream & outputStream) const {
        outputStream << "mtsCommandReadProxy: " << Name << ", " << CommandId << std::endl;
        outputStream << "Currently " << (this->IsEnabled() ? "enabled" : "disabled");
    }

    /*! Set an argument prototype */
    void SetArgumentPrototype(mtsGenericObject * argumentPrototype) {
        this->ArgumentPrototype = argumentPrototype;
    }
};

#endif // _mtsCommandReadProxy_h
