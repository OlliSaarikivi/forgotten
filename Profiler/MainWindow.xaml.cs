using Profiler.ToolingService;
using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceModel;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.DataVisualization.Charting;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Profiler
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        MetricsSubscribeServiceClient _client;

        class MetricsEventsHandler : IMetricsSubscribeServiceCallback
        {
            MainWindow main;

            public MetricsEventsHandler(MainWindow main)
                : base()
            {
                this.main = main;
            }

            public void OnTick(TickTimings tick)
            {
                main.AddTick(tick);
            }
        }

        class MetricsClient : DuplexClientBase<IMetricsSubscribeService>, IMetricsSubscribeService
        {
            MetricsClient(InstanceContext context) : base(context) { }

            public void Subscribe()
            {
                base.Channel.Subscribe();
            }

            public void Unsubscribe()
            {
                base.Channel.Unsubscribe();
            }
        }

        class Phase
        {
            Dictionary<string, ProcessTimingSeries> timingSeries = new Dictionary<string, ProcessTimingSeries>();
            Chart chart;
            ProcessTimingSeries phaseSeries;
            DateTime lastUpdated = DateTime.MinValue;
            int currentTick = 1;

            public Phase(TabControl chartTabs, Style chartStyle, string phaseName)
            {
                var grid = new Grid();
                var item = new TabItem
                {
                    Header = phaseName,
                };
                chart = new Chart
                {
                    Title = phaseName,
                    Style = chartStyle,
                };
                chart.Axes.Add(new LinearAxis
                {
                    Orientation = AxisOrientation.X
                });
                chart.Axes.Add(new LinearAxis
                {
                    Orientation = AxisOrientation.Y,
                    Minimum = 0,
                    Maximum = 16,
                });
                phaseSeries = new ProcessTimingSeries();
                var phaseLineSeries = new LineSeries
                {
                    Title = "Total",
                    ItemsSource = phaseSeries.Data,
                    IndependentValuePath = "Tick",
                    DependentValuePath = "Duration",
                };
                chart.Series.Add(phaseLineSeries);
                grid.Children.Add(chart);
                item.Content = grid;
                chartTabs.Items.Add(item);
            }

            public void AddTick(TickTimings tick)
            {
                if (DateTime.Now - lastUpdated > TimeSpan.FromSeconds(0.5))
                {
                    lastUpdated = DateTime.Now;
                    foreach (var process in tick.ProcessTimings)
                    {
                        ProcessTimingSeries series;
                        if (!timingSeries.TryGetValue(process.Name, out series))
                        {
                            series = new ProcessTimingSeries();
                            var lineSeries = new LineSeries
                            {
                                Title = process.Name,
                                ItemsSource = series.Data,
                                IndependentValuePath = "Tick",
                                DependentValuePath = "Duration",
                            };
                            chart.Series.Add(lineSeries);
                            timingSeries.Add(process.Name, series);
                        }
                        series.Add(currentTick, process.DurationNanos);
                    }
                    phaseSeries.Add(currentTick, tick.TotalDurationNanos);
                    currentTick += 1;
                }
            }
        }

        Dictionary<string, Phase> phases = new Dictionary<string, Phase>();

        public void AddTick(TickTimings tick)
        {
            Phase phase;
            if (!phases.TryGetValue(tick.Phase, out phase))
            {
                phase = new Phase(ChartTabs, (Style)Resources["MyChartStyle"], tick.Phase);
                phases.Add(tick.Phase, phase);
            }
            phase.AddTick(tick);
        }

        public MainWindow()
        {
            InitializeComponent();

            var handler = new MetricsEventsHandler(this);
            _client = new MetricsSubscribeServiceClient(new InstanceContext(handler));
            _client.Subscribe();
        }

        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            try
            {
                _client.Unsubscribe();
            }
            catch { }
        }
    }
}
