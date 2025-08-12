import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

def plot_sensor_data(csv_file='sensor_log.csv'):
    """
    Reads sensor data from a CSV file and generates a multi-panel plot.

    Args:
        csv_file (str): The path to the CSV file containing the sensor data.
    """
    try:
        # Read the CSV data into a pandas DataFrame.
        # 'parse_dates' tells pandas to automatically interpret the first column as dates.
        df = pd.read_csv(csv_file, parse_dates=[0], index_col=0)
    except FileNotFoundError:
        print(f"Error: The file '{csv_file}' was not found.")
        print("Please make sure the script is in the same directory as your log file.")
        return
    except Exception as e:
        print(f"An error occurred while reading the CSV file: {e}")
        return

    if df.empty:
        print("The CSV file is empty. No data to plot.")
        return

    print(f"Successfully loaded {len(df)} data points from '{csv_file}'.")

    # --- Plotting ---
    # Create a figure and a set of subplots. 3 rows, 1 column.
    # 'figsize' controls the size of the output image.
    # 'sharex=True' makes all subplots share the same x-axis (time).
    fig, axes = plt.subplots(nrows=3, ncols=1, figsize=(12, 10), sharex=True)

    # Plot CO2
    axes[0].plot(df.index, df['Avg_CO2_ppm'], marker='o', linestyle='-', color='b')
    axes[0].set_title('CO2 Level')
    axes[0].set_ylabel('CO2 (ppm)')
    axes[0].grid(True)

    # Plot Temperature
    axes[1].plot(df.index, df['Avg_Temp_C'], marker='o', linestyle='-', color='r')
    axes[1].set_title('Temperature')
    axes[1].set_ylabel('Temperature (°C)')
    axes[1].grid(True)

    # Plot Humidity
    axes[2].plot(df.index, df['Avg_Humidity_RH'], marker='o', linestyle='-', color='g')
    axes[2].set_title('Humidity')
    axes[2].set_ylabel('Humidity (%RH)')
    axes[2].grid(True)

    # --- Formatting the X-axis (Date/Time) ---
    # Improve the date formatting on the x-axis.
    fig.autofmt_xdate() # Rotates and aligns the date labels nicely.
    date_format = mdates.DateFormatter('%Y-%m-%d %H:%M')
    axes[2].xaxis.set_major_formatter(date_format)
    plt.xlabel('Date and Time')

    # Add a main title to the entire figure
    fig.suptitle('SCD41 Sensor Readings over Time', fontsize=16)
    plt.tight_layout(rect=[0, 0.03, 1, 0.95]) # Adjust layout to make room for the suptitle

    # Save the plot to a file
    output_filename = 'sensor_plot.png'
    plt.savefig(output_filename)
    print(f"Plot successfully saved to '{output_filename}'")

if __name__ == '__main__':
    plot_sensor_data()
