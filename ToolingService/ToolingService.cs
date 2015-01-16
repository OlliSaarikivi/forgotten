using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Collections.Concurrent;
using System.ServiceModel;

namespace ToolingService
{
    [ServiceBehavior(InstanceContextMode = InstanceContextMode.PerSession)]
    class ToolingService : IMetricsPublishService, IMetricsSubscribeService
    {
        public event TickEvent _tickHandlers;

        public void Subscribe()
        {
            var subscriber = OperationContext.Current.GetCallbackChannel<IMetricsEvents>();
            _tickHandlers += subscriber.OnTick;
        }

        public void Unsubscribe()
        {
            var subscriber = OperationContext.Current.GetCallbackChannel<IMetricsEvents>();
            _tickHandlers -= subscriber.OnTick;
        }

        public void PostTick(TickTimings tick)
        {
            var handlers = _tickHandlers;
            if (handlers != null)
            {
                handlers(tick);
            }
        }
    }
}
