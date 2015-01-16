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

            public MetricsEventsHandler(MainWindow main) : base()
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
            StackedAreaSeries seriesControl;

            public Phase(TabControl chartTabs, string phaseName)
            {
                var grid = new Grid();
                var chart = new Chart
                {
                    Title = phaseName,
                };
                chart.Axes.Add(new LinearAxis
                {
                    Orientation = AxisOrientation.X
                });
                chart.Axes.Add(new LinearAxis
                {
                    Orientation = AxisOrientation.Y
                });
                seriesControl = new StackedAreaSeries();
                chart.Series.Add(seriesControl);
                grid.Children.Add(chart);
                chartTabs.Items.Add(grid);
            }

            public void AddTick(TickTimings tick)
            {
                foreach (var process in tick.ProcessTimings)
                {
                    ProcessTimingSeries series;
                    if (!timingSeries.TryGetValue(process.Name, out series))
                    {
                        series = new ProcessTimingSeries();
                        var binding = new Binding("Data");
                        binding.Source = series;
                        var definition = new SeriesDefinition
                        {
                            DataContext = series,
                            DependentValueBinding = binding
                        };
                        seriesControl.SeriesDefinitions.Add(definition);
                        timingSeries.Add(process.Name, series);
                    }
                    series.Add(process.DurationNanos);
                }
            }
        }

        Dictionary<string, Phase> phases = new Dictionary<string, Phase>();

        public void AddTick(TickTimings tick)
        {
            Phase phase;
            if (!phases.TryGetValue(tick.Phase, out phase))
            {
                phase = new Phase(ChartTabs, tick.Phase);
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
            _client.Unsubscribe();
        }
    }
}
