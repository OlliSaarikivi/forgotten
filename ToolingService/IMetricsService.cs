using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ServiceModel;
using System.ServiceModel.Description;
using System.Runtime.Serialization;

namespace ToolingService
{
    delegate void TickEvent(TickTimings tick);
    interface IMetricsEvents
    {
        [OperationContract(IsOneWay = true)]
        void OnTick(TickTimings tick);
    }

    [ServiceContract(SessionMode = SessionMode.Required, CallbackContract = typeof(IMetricsEvents))]
    interface IMetricsSubscribeService
    {
        [OperationContract(IsOneWay = false, IsInitiating = true)]
        void Subscribe();
        [OperationContract(IsOneWay = false, IsTerminating = true)]
        void Unsubscribe();
    }

    [ServiceContract]
    interface IMetricsPublishService
    {
        [OperationContract(IsOneWay = true)]
        void PostTick(TickTimings tick);
    }

    [DataContract]
    public class TickTimings
    {
        [DataMember]
        public string Phase { get; set; }
        [DataMember]
        public float Step { get; set; }
        [DataMember]
        public uint TotalDurationNanos { get; set; }
        [DataMember]
        public List<ProcessTiming> ProcessTimings { get; set; }
    }

    [DataContract]
    public class ProcessTiming
    {
        [DataMember]
        public string Name { get; set; }
        [DataMember]
        public uint DurationNanos { get; set; }
    }
}
