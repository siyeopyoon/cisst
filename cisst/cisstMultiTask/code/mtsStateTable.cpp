/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
$Id$

Author(s):  Ankur Kapoor, Min Yang Jung, Anton Deguet
Created on: 2004-04-30

(C) Copyright 2004-2011 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstCommon/cmnAssert.h>
#include <cisstOSAbstraction/osaTimeServer.h>
#include <cisstMultiTask/mtsStateTable.h>
#include <cisstMultiTask/mtsTaskManager.h>
#include <cisstMultiTask/mtsCollectorState.h>
#if CISST_HAS_SAFETY_PLUGINS
#include <cisstMultiTask/mtsMonitorFilterBase.h>
#endif

#include <iostream>
#include <string>

const std::string mtsStateTable::NamesOfDefaultElements::Tic = "Tic";
const std::string mtsStateTable::NamesOfDefaultElements::Toc = "Toc";
const std::string mtsStateTable::NamesOfDefaultElements::Period = "Period";
const std::string mtsStateTable::NamesOfDefaultElements::PeriodStatistics = "PeriodStatistics";
#if CISST_HAS_SAFETY_PLUGINS
const std::string mtsStateTable::NamesOfDefaultElements::ExecTimeUser = "ExecTimeUser";
const std::string mtsStateTable::NamesOfDefaultElements::ExecTimeTotal = "ExecTimeTotal";
#endif

const mtsStateDataId mtsStateTable::INVALID_STATEVECTOR_ID = -1;

void mtsStateTable::IndexRange::ToStreamRaw(std::ostream & outputStream, const char delimiter,
                                            bool headerOnly, const std::string & headerPrefix) const
{
    mtsGenericObject::ToStreamRaw(outputStream, delimiter, headerOnly, headerPrefix);
    outputStream << delimiter;
    First.ToStreamRaw(outputStream, delimiter, headerOnly, headerPrefix + "first-");
    outputStream << delimiter;
    Last.ToStreamRaw(outputStream, delimiter, headerOnly, headerPrefix + "last-");
}


mtsStateTable::mtsStateTable(size_t size, const std::string & name):
    HistoryLength(size),
    IndexWriter(0),
    IndexReader(0),
    IndexDelayed(0),
    AutomaticAdvanceFlag(true),
    StateVector(0),
    StateVectorDataNames(0),
    Ticks(size, mtsStateIndex::TimeTicksType(0)),
    Tic(0.0),
    Toc(0.0),
    Period(0.0),
    SumOfPeriods(0.0),
    AveragePeriod(0.0),
    Name(name)
#if CISST_HAS_SAFETY_PLUGINS
    , ExecTimeUser(0.0)
    , ExecTimeTotal(0.0)
#endif
{
    // make sure history length is at least 3
    if (this->HistoryLength < 3) {
        CMN_LOG_CLASS_INIT_VERBOSE << "constructor: history lenght sets to 3 (minimum required)" << std::endl;
        this->HistoryLength = 3;
    }

    // set the default number of elements for data collection batch
    this->DataCollection.BatchSize = this->HistoryLength / 3;
    if (this->DataCollection.BatchSize == 0) {
        this->DataCollection.BatchSize = 1;
    }
    this->DataCollection.TimeIntervalForProgressEvent = 1.0 * cmn_s;
    this->DataCollection.BatchRange.SetValid(true);
    this->DataCollection.BatchRange.SetAutomaticTimestamp(false);

    // Get a pointer to the time server
    TimeServer = &mtsTaskManager::GetInstance()->GetTimeServer();

    // Add Tic and Toc to the StateTable. We add Toc first, to make things easier
    // in mtsStateTable::Advance.  Do not change this.
    TocId = NewElement(mtsStateTable::NamesOfDefaultElements::Toc, &Toc);
    TicId = NewElement(mtsStateTable::NamesOfDefaultElements::Tic, &Tic);
    // Add Period to the StateTable.
    PeriodId = NewElement(mtsStateTable::NamesOfDefaultElements::Period, &Period);

    // Add statistics
    NewElement(mtsStateTable::NamesOfDefaultElements::PeriodStatistics, &PeriodStats);

#if CISST_HAS_SAFETY_PLUGINS
    NewElement(mtsStateTable::NamesOfDefaultElements::ExecTimeUser, &ExecTimeUser);
    NewElement(mtsStateTable::NamesOfDefaultElements::ExecTimeTotal, &ExecTimeTotal);
#endif
}

mtsStateTable::~mtsStateTable()
{
}


/* All the const methods that can be called from reader or writer */
mtsStateIndex mtsStateTable::GetIndexReader(void) const {
    size_t tmp = IndexReader;
    return mtsStateIndex(this->Tic, tmp, Ticks[tmp], HistoryLength);
}

mtsStateIndex mtsStateTable::GetIndexDelayed(void) const {
    size_t tmp = IndexDelayed;
    return mtsStateIndex(this->Tic, tmp, Ticks[tmp], HistoryLength);
}

size_t mtsStateTable::SetDelay(size_t newDelay) {
    size_t currentDelay = this->Delay;
    this->Delay = newDelay;
    return currentDelay;
}


mtsStateTable::AccessorBase * mtsStateTable::GetAccessor(const std::string & name) const
{
    for (size_t i = 0; i < StateVectorDataNames.size(); i++) {
        if (name == StateVectorDataNames[i]) {
            return StateVectorAccessors[i];
        }
    }
    return 0;
}


mtsStateTable::AccessorBase * mtsStateTable::GetAccessor(const char * name) const
{
    return GetAccessor(std::string(name));
}


/* All the non-const methods that can be called from writer only */
mtsStateIndex mtsStateTable::GetIndexWriter(void) const {
    return mtsStateIndex(this->Tic, IndexWriter, Ticks[IndexWriter], HistoryLength);
}


bool mtsStateTable::Write(mtsStateDataId id, const mtsGenericObject & object) {
    bool result;
    CMN_ASSERT(id != -1);
    if (id == -1) {
        CMN_LOG_CLASS_INIT_ERROR << "Write: object must be created using NewElement " << std::endl;
        return false;
    }
    if (!StateVector[id]) {
        CMN_LOG_CLASS_INIT_ERROR << "Write: no state data array corresponding to given id: " << id << std::endl;
        return false;
    }
    result = StateVector[id]->Set(IndexWriter, object);
    if (!result) {
        CMN_LOG_CLASS_INIT_ERROR << "Write: error setting data array value in id: " << id << std::endl;
    }
    return result;
}


void mtsStateTable::Start(void) {
    if (TimeServer) {
        Tic = TimeServer->GetRelativeTime(); // in seconds
        // Since IndexReader and IndexWriter are initialized to 0,
        // the first period will be 0
        mtsDouble oldTic;
        StateVector[TicId]->Get(IndexReader, oldTic);
        Period = Tic - oldTic;  // in seconds
    }
}


void mtsStateTable::StartIfAutomatic(void) {
    if (this->AutomaticAdvanceFlag) {
        this->Start();
    }
}


void mtsStateTable::Advance(void) {
    size_t i;
    size_t tmpIndex;
    // newIndexWriter is the next row of the State Table.  Note that this
    // also corresponds to the row with the oldest data.
    size_t newIndexWriter = (IndexWriter + 1) % HistoryLength;

    // Update SumOfPeriods (add newest and subtract oldest)
    SumOfPeriods += Period;
    // If the table is full (all entries valid), subtract the oldest one
    if (Ticks[IndexWriter] == (Ticks[newIndexWriter] + HistoryLength - 1)) {
        mtsDouble oldPeriod;
        StateVector[PeriodId]->Get(newIndexWriter, oldPeriod);
        SumOfPeriods -= oldPeriod;
        AveragePeriod = SumOfPeriods / (HistoryLength-1);
    }
    else if (Ticks[IndexWriter] > 0) {
        AveragePeriod = SumOfPeriods / Ticks[IndexWriter];
    }

    //Update Period Statistics
    PeriodStats.AddSample(Period.Data);

    /* If for all cases, IndexReader is behind IndexWriter, we don't
    need critical sections. This is based on the assumption that
    there is only one Writer that has access to Advance method and
    this is the only place where IndexReader is modified.  Oh ya!
    another assumption, we don't have a buffer of size less than 3.
    */

    /* So far IndexReader < IndexWriter.
    The following block doesn't modify IndexReader,
    so the above condition still holds.
    */
    tmpIndex = IndexWriter;

    // Write data in the state table from the different state data objects.
    // Note that we start at TicId, which should correspond to the second
    // element in the array (after Toc).
    for (i = TicId; i < StateVector.size(); i++) {
        if (StateVectorElements[i]) {
            StateVectorElements[i]->SetTimestampIfAutomatic(Tic.Data);
            Write(i, *(StateVectorElements[i]));
        }
    }
    // Get the Toc value and write it to the state table.
    if (TimeServer) {
        Toc = TimeServer->GetRelativeTime(); // in seconds
    }
    Write(TocId, Toc);
    // now increment the IndexWriter and set its Tick value
    IndexWriter = newIndexWriter;
    Ticks[IndexWriter] = Ticks[tmpIndex] + 1;
    // move index reader to recently written data
    IndexReader = tmpIndex;

    // compute index delayed, ideally a valid index
    if (IndexReader > Delay) {
        IndexDelayed = IndexReader - Delay;
    }

    // data collection, test if we are currently collecting
    if (!this->DataCollection.Collecting) {
        // check if a start time is set and has arrived
        if ((this->DataCollection.StartTime != 0.0)
            && (this->Tic >= this->DataCollection.StartTime)) {
            // start collection
            CMN_LOG_CLASS_RUN_DEBUG << "Advance: data collection started at " << this->Tic << std::endl;
            // send collection started event
            this->DataCollection.CollectionStarted();
            // reset start time
            this->DataCollection.StartTime = 0.0;
            this->DataCollection.BatchRange.First = this->GetIndexReader();
            this->DataCollection.BatchCounter = 0;
            this->DataCollection.Collecting = true;
            // reset counter for event
            this->DataCollection.CounterForEvent = 0;
            this->DataCollection.TimeOfLastProgressEvent = this->Tic;
        }
    }
    // are we collecting?
    if (this->DataCollection.Collecting) {
        // check if a stop time is set and has arrived
        if ((this->DataCollection.StopTime != 0.0)
            && (this->Tic >= this->DataCollection.StopTime)) {
            // stop collection
            CMN_LOG_CLASS_RUN_DEBUG << "Advance: data collection stopped at " << this->Tic << std::endl;
            // reset start time
            this->DataCollection.StopTime = 0.0;
            this->DataCollection.BatchRange.Last = this->GetIndexReader();
            // request data actual for range collection
            this->DataCollection.BatchRange.SetTimestamp(this->Tic);
            this->DataCollection.BatchReady(this->DataCollection.BatchRange);
            // send collection stopped event
            this->DataCollection.CollectionStopped(mtsUInt(this->DataCollection.CounterForEvent));
            this->DataCollection.CounterForEvent = 0;
            // stop collecting
            this->DataCollection.Collecting = false;
        } else {
            // still collecting
            this->DataCollection.BatchCounter++;
            this->DataCollection.CounterForEvent++;
            // check if we have collected enough element for actual collection
            if (this->DataCollection.BatchCounter >= this->DataCollection.BatchSize) {
                CMN_LOG_CLASS_RUN_DEBUG << "Advance: " << this->DataCollection.BatchCounter
                                        << " element(s) available for data collection" << std::endl;
                this->DataCollection.BatchRange.Last = this->GetIndexReader();
                // request data actual for range collection
                this->DataCollection.BatchRange.SetTimestamp(this->Tic);
                this->DataCollection.BatchReady(this->DataCollection.BatchRange);
                this->DataCollection.BatchCounter = 0;
                this->DataCollection.BatchRange.First = this->GetIndexWriter();
            }
            // check if we have spent enough time for a progress event
            if ((this->Tic - this->DataCollection.TimeOfLastProgressEvent) >= this->DataCollection.TimeIntervalForProgressEvent) {
                this->DataCollection.Progress(mtsUInt(this->DataCollection.CounterForEvent));
                this->DataCollection.CounterForEvent = 0;
                this->DataCollection.TimeOfLastProgressEvent = this->Tic;
            }
        }
    }

#if CISST_HAS_SAFETY_PLUGINS
#define PROCESS_FILTERS( _name )\
    if (!MonitorFilters._name.empty())\
        for (size_t i = 0; i < MonitorFilters._name.size(); ++i)\
            MonitorFilters._name[i]->DoFiltering(false);
    // Process filters sequentially
    PROCESS_FILTERS(Features);
    PROCESS_FILTERS(FeatureVectors);
    PROCESS_FILTERS(Symptoms);
    PROCESS_FILTERS(SymptomVectors);
    PROCESS_FILTERS(FaultDetectors);
#undef PROCESS_FILTERS
#endif
}


void mtsStateTable::AdvanceIfAutomatic(void) {
    if (this->AutomaticAdvanceFlag) {
        this->Advance();
    }
}


void mtsStateTable::Cleanup(void) {
    CMN_LOG_CLASS_INIT_DEBUG << "Cleanup: state table \"" << this->Name << "\"" << std::endl;
    // if the state table is still set to collect data, send error message, should have been stopped
    if (this->DataCollection.Collecting) {
        CMN_LOG_CLASS_INIT_ERROR << "Cleanup: data collection for state table \"" << this->Name
                                 << "\" has not been stopped.  It is possible that the state collector will look for this state table after it has been deleted." << std::endl;
    }

#if CISST_HAS_SAFETY_PLUGINS
#define CLEANUP_FILTERS( _name )\
    if (!MonitorFilters._name.empty())\
        for (size_t i = 0; i < MonitorFilters._name.size(); ++i)\
            delete MonitorFilters._name[i];\
    MonitorFilters._name.clear();
    // Process filters sequentially
    CLEANUP_FILTERS(Features);
    CLEANUP_FILTERS(FeatureVectors);
    CLEANUP_FILTERS(Symptoms);
    CLEANUP_FILTERS(SymptomVectors);
    CLEANUP_FILTERS(FaultDetectors);
#undef CLEANUP_FILTERS
#endif
}


void mtsStateTable::ToStream(std::ostream & outputStream) const {
    outputStream << "State Table: " << this->GetName() << std::endl;
    size_t i;
    outputStream << "Ticks : ";
    for (i = 0; i < StateVector.size() - 1; i++) {
        if (!StateVectorDataNames[i].empty())
            outputStream << "[" << i << "]"
                         << StateVectorDataNames[i].c_str() << " : ";
    }
    outputStream << "[" << i << "]"
                 << StateVectorDataNames[i].c_str() << std::endl;
#if 0
    // the following is a data dump, it should go in ToStreamRaw
    for (i = 0; i < HistoryLength; i++) {
        out << i << ": ";
        out << Ticks[i] << ": ";
        for (unsigned int j = 0; j < StateVector.size(); j++)  {
            if (StateVector[j]) {
                out << " [" << j << "] "
                    << (*StateVector[j])[i]
                    << " : ";
            }
        }
        if (i == IndexReader)
            out << "<-- Index for Reader";
        if (i == IndexWriter)
            out << "<== Index for Writer";
        out << std::endl;
    }
#endif
}


void mtsStateTable::Debug(std::ostream & out, unsigned int * listColumn, unsigned int number) const {
    unsigned int i, j;

    for (i = 0; i < number; i++) {
        if (!StateVectorDataNames[listColumn[i]].empty()) {
            out << "["
                << listColumn[i] << "]" <<
                StateVectorDataNames[listColumn[i]].c_str() << " : ";
        }
    }
    out << std::endl;

    for (i = 0; i < HistoryLength; i++) {
        out << i << " ";
        out << Ticks[i] << " ";
        for (j = 0; j < number; j++) {
            if (listColumn[j] < StateVector.size() && StateVector[listColumn[j]]) {
                out << " [" << listColumn[j] << "] "
                    <<(*StateVector[listColumn[j]])[i] << " : ";
            }
        }
        if (i == IndexReader) {
            out << "<-- Index for Reader";
        }
        if (i == IndexWriter) {
            out << "<== Index for Writer";
        }
        out << std::endl;
    }
}

// This method is to dump the state data table in the csv format, allowing easy import into matlab.
// Assumes that individual columns are also printed in csv format.

// By default print all rows, if nonZeroOnly == true then print only those rows which have a nonzero Ticks
// value i.e, those rows that have been written to at least once.
void mtsStateTable::CSVWrite(std::ostream& out, bool nonZeroOnly) {
    unsigned int i;
    for (i = 0; i < HistoryLength; i++) {
        bool toSave = true;
        if (nonZeroOnly && Ticks[i] ==0) toSave = false;
        if (toSave) {
            out << i << " " << Ticks[i] << " ";
            for (unsigned int j = 0; j < StateVector.size(); j++)  {
                if (StateVector[j]) {
                    out << (*StateVector[j])[i] << " ";
                }
            }
            out << std::endl;
        }
    }
}

void mtsStateTable::CSVWrite(std::ostream& out, unsigned int *listColumn, unsigned int number, bool nonZeroOnly) {
    unsigned int i, j;

    for (i = 0; i < HistoryLength; i++) {
        bool toSave = true;
        if (nonZeroOnly && Ticks[i] ==0) toSave = false;
        if (toSave) {
            out << i << " " << Ticks[i] << " ";
            for (j = 0; j < number; j++) {
                if (listColumn[j] < StateVector.size() && StateVector[listColumn[j]]) {
                    out << (*StateVector[listColumn[j]])[i] << " ";
                }
            }
            out << std::endl;
        }
    }
}

void mtsStateTable::CSVWrite(std::ostream& out, mtsGenericObject ** listColumn, unsigned int number, bool nonZeroOnly)
{
    unsigned int *listColumnId = new unsigned int[number];
    for (unsigned int i = 0; i < number; i++) {
        listColumnId[i] = StateVectorElements.size();  // init to invalid value
        for (unsigned int j = 0; j < StateVectorElements.size(); j++) {
            if (StateVectorElements[j] == listColumn[i])
                listColumnId[i] = j;
        }
    }
    CSVWrite(out, listColumnId, number, nonZeroOnly);
    delete [] listColumnId;
}


int mtsStateTable::GetStateVectorID(const std::string & dataName) const
{
    for (size_t i = 0; i < StateVectorDataNames.size(); i++) {
        if (StateVectorDataNames[i] == dataName) {
            return i;
        }
    }
    return INVALID_STATEVECTOR_ID;
}


void mtsStateTable::DataCollectionStart(const mtsDouble & delay)
{
    CMN_LOG_CLASS_RUN_DEBUG << "DataCollectionStart: received request to start data collection in "
                            << delay.Data << " seconds" << std::endl;
    // set start time based on current time and delay
    const double startTime = this->TimeServer->GetRelativeTime() + delay.Data;
    // if we are not yet collection
    if (!this->DataCollection.Collecting) {
        // if there is no collection scheduled
        if (this->DataCollection.StartTime == 0) {
            // set time to start
            CMN_LOG_CLASS_RUN_DEBUG << "DataCollectionStart: data collection scheduled to start at "
                                    << startTime << " for state table \"" << this->GetName() << "\"" << std::endl;
            this->DataCollection.StartTime = startTime;
        } else {
            // we are set to collect but later, advance the collection
            // time.  this is a conservative approach, if we have 2
            // different start times, we take the earliest of both to
            // collect more data.
            if (this->DataCollection.StartTime > startTime) {
                CMN_LOG_CLASS_RUN_DEBUG << "DataCollectionStart: data collection scheduled to start at "
                                        << startTime << " (moved forward) for state table \"" << this->GetName() << "\"" << std::endl;
                this->DataCollection.StartTime = startTime;
            } else {
                CMN_LOG_CLASS_RUN_WARNING << "DataCollectionStart: received a new request to start data collection after previous request, ignored"
                                          << std::endl;
            }
        }
    } else {
        // we are already collecting, see if a time to stop has been set
        if (this->DataCollection.StopTime == 0) {
            CMN_LOG_CLASS_RUN_WARNING << "DataCollectionStart: received request to start collection while still set to collect, ignored" << std::endl;
        } else {
            // make sure the request to start comes after the next scheduled stop
            if (startTime <= this->DataCollection.StartTime) {
                CMN_LOG_CLASS_RUN_WARNING << "DataCollectionStart: received request to start collection before next scheduled stop, ignored" << std::endl;
            } else {
                // this will schedule a start after the scheduled stop
                CMN_LOG_CLASS_RUN_DEBUG << "DataCollectionStart: data collection scheduled to start at "
                                        << startTime << " (after a scheduled stop) for state table \""
                                        << this->GetName() << "\"" << std::endl;
                this->DataCollection.StartTime = startTime;
            }
        }
    }
}


void mtsStateTable::DataCollectionStop(const mtsDouble & delay)
{
    CMN_LOG_CLASS_RUN_DEBUG << "DataCollectionStop: received request to stop data collection in "
                            << delay.Data << " seconds for state table \""
                            << this->GetName() << "\"" << std::endl;
    // set stop time based on current time and delay
    const double stopTime = this->TimeServer->GetRelativeTime() + delay.Data;
    // check is there is already a stop time scheduled
    if (this->DataCollection.StopTime == 0) {
        CMN_LOG_CLASS_RUN_DEBUG << "DataCollectionStop: data collection scheduled to stop at "
                                << stopTime << " for state table \""
                                << this->GetName() << "\"" << std::endl;
        DataCollection.StopTime = stopTime;
    } else {
        // we are set to stop but earlier, delay the collection stop
        // time.  this is a conservative approach, if we have 2
        // different stop times, we take the latest of both to collect
        // more data.
        if (this->DataCollection.StopTime < stopTime) {
            CMN_LOG_CLASS_RUN_DEBUG << "DataCollectionStop: data collection scheduled to stop at "
                                    << stopTime << " (moved back) for state table \""
                                    << this->GetName() << "\"" << std::endl;
            DataCollection.StopTime = stopTime;
        } else {
            CMN_LOG_CLASS_RUN_WARNING << "DataCollectionStop: received a new request to stop data collection before previous request, ignored"
                                      << std::endl;
        }
    }
}

#if CISST_HAS_SAFETY_PLUGINS
bool mtsStateTable::AddFilter(mtsMonitorFilterBase * filter)
{
    // Filter validity check
    if (!filter) {
        CMN_LOG_CLASS_RUN_ERROR << "AddFilter: invalid filter" << std::endl;
        return false;
    } else {
        mtsMonitorFilterBase::FILTER_TYPE filterType = filter->GetFilterType();
        if (filterType < mtsMonitorFilterBase::FEATURE || filterType > mtsMonitorFilterBase::FAULT_DETECTOR) {
            CMN_LOG_CLASS_RUN_ERROR << "AddFilter: invalid filter type: \"" << filter->GetFilterName() << "\"" << std::endl;
            return false;
        }
    }

    const std::string filterName = filter->GetFilterName();

    // Check if all inputs of the filter exists
    std::string inputElementName;
    mtsStateDataId inputElementId;
    const size_t numOfInputs = filter->GetNumberOfInputs();
    for (size_t i = 0; i < numOfInputs; ++i) {
        inputElementName = filter->GetInputSignalName(i);
        inputElementId = GetStateVectorID(inputElementName);
        if (inputElementId == INVALID_STATEVECTOR_ID) {
            CMN_LOG_CLASS_RUN_ERROR << "AddFilter: Input element \"" << inputElementName << "\" required by filter \""
                                    << filterName << "\" is missing from state table \"" << this->GetName() << "\"" 
                                    << std::endl;
            return false;
        } else {
            mtsMonitorFilterBase::SignalElement * inputSignal = filter->GetInputSignalElement(i);
            CMN_ASSERT(inputSignal);
            inputSignal->SetStateDataId(inputElementId);
        }
    }

    // Add all outputs of the filter to this state table
    std::string outputElementName;
    mtsStateDataId outputElementId;
    mtsMonitorFilterBase::SignalElement::SIGNAL_TYPE outputSignalType;
    const size_t numOfOutputs = filter->GetNumberOfOutputs();
    for (size_t i = 0; i < numOfOutputs; ++i) {
        mtsMonitorFilterBase::SignalElement * outputSignal = filter->GetOutputSignalElement(i);
        CMN_ASSERT(outputSignal);
        outputElementName = filter->GetOutputSignalName(i);

        // MJ TODO: adding a new element on the fly may not be thread-safe -- need double check.
        outputSignalType = outputSignal->GetSignalType();
        if (outputSignalType == mtsMonitorFilterBase::SignalElement::SCALAR) {
            outputElementId = NewElement(outputElementName, &outputSignal->Placeholder);
        } else {
            outputElementId = NewElement(outputElementName, &outputSignal->PlaceholderVector);
        }
        outputSignal->SetStateDataId(outputElementId);

        CMN_LOG_CLASS_RUN_DEBUG << "AddFilter: Output element \"" << outputElementName << "\" generated by filter \""
                                << filterName << "\" is added to state table \"" << this->GetName() << "\"" 
                                << std::endl;
    }

    switch (filter->GetFilterType()) {
        case mtsMonitorFilterBase::FEATURE:
            MonitorFilters.Features.push_back(filter);
            break;
        case mtsMonitorFilterBase::FEATURE_VECTOR:
            MonitorFilters.FeatureVectors.push_back(filter);
            break;
        case mtsMonitorFilterBase::SYMPTOM:
            MonitorFilters.Symptoms.push_back(filter);
            break;
        case mtsMonitorFilterBase::SYMPTOM_VECTOR:
            MonitorFilters.SymptomVectors.push_back(filter);
            break;
        case mtsMonitorFilterBase::FAULT_DETECTOR:
            MonitorFilters.FaultDetectors.push_back(filter);
            break;
        case mtsMonitorFilterBase::INVALID:
        default:
            CMN_LOG_CLASS_RUN_ERROR << "AddFilter: invalid filter type: \"" << filter->GetFilterName() << "\"" << std::endl;
            return false;
    }

    // Remember where the filter is attached to
    filter->SetStateTableInstance(this);

    CMN_LOG_CLASS_RUN_DEBUG << "AddFilter: added new filter: " << filterName << std::endl;

    return true;
}

// MJ TEMP
double mtsStateTable::GetNewValueScalar(const mtsStateDataId id, double & timeStamp) const
{
    mtsDouble value;
    StateVector[id]->Get(IndexReader, value);
    value.GetTimestamp(timeStamp);

    return value.Data;
}

mtsDoubleVec mtsStateTable::GetNewValueVector(const mtsStateDataId id, double & timeStamp) const
{
    mtsDoubleVec vec;
    StateVector[id]->Get(IndexReader, vec);
    vec.GetTimestamp(timeStamp);

    return vec;
}
#endif

