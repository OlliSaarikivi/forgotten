using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using RGiesecke.DllExport;
using ToolingConnection.ToolingService;

namespace ToolingConnection
{
    class Game
    {
        static bool _started = false;
        static MetricsPublishServiceClient _client;
        static TickTimings _currentTick;

        [DllExport("metrics_start", CallingConvention.Cdecl)]
        public static bool Start()
        {
            _client = new MetricsPublishServiceClient();
            _client.Open();
            return true;
        }

        [DllExport("metrics_stop", CallingConvention.Cdecl)]
        public static void Stop()
        {
            if (!_started)
            {
                return;
            }
            _client.Close();
            _client = null;
            _started = false;
        }

        [DllExport("metrics_processTiming", CallingConvention.Cdecl)]
        public static void ProcessTiming([MarshalAs(UnmanagedType.LPStr)] string name, UInt32 durationNanos)
        {
            if (_currentTick != null)
            {
                _currentTick.ProcessTimings.Add(new ProcessTiming
                {
                    Name = name,
                    DurationNanos = durationNanos,
                });
            }
        }

        [DllExport("metrics_beginTick", CallingConvention.Cdecl)]
        public static void BeginTick([MarshalAs(UnmanagedType.LPStr)] string phase, float step)
        {
            _currentTick = new TickTimings
            {
                Phase = phase,
                Step = step,
            };
        }

        [DllExport("metrics_endTick", CallingConvention.Cdecl)]
        public static void EndTick(UInt32 totalDurationNanos)
        {
            if (_currentTick != null)
            {
                _currentTick.TotalDurationNanos = totalDurationNanos;
                _client.PostTick(_currentTick);
                _currentTick = null;
            }
        }
    }
}
