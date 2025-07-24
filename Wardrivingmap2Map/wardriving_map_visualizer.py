import folium
import pandas as pd
import sys
import os
import argparse
import webbrowser
from folium.plugins import MarkerCluster
from folium.plugins import TimestampedGeoJson

RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
BLUE = "\033[34m"
RESET = "\033[0m"

header = [
    "Date",
    "Time_UTC",
    "Latitude",
    "Longitude",
    "Altitude",
    "Satellites",
    "Accuracy",
    "SSID",
    "BSSID",
    "Channel",
    "RSSI",
    "Encryption",
]
int_columns = ["satellites", "accuracy", "channel", "rssi"]
float_columns = ["latitude", "longitude", "altitude"]


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="generate and view a wardriving map from csv data."
    )
    parser.add_argument(
        "csv_file", help="path to the csv file containing the wardriving data."
    )
    parser.add_argument(
        "--print_data",
        type=str,
        default="nah",
        choices=["True", "False", "true", "false", "1", "0"],
        help="0/false: print the csv data and exit without saving, 1/true: print the csv data and save html_map",
    )
    parser.add_argument(
        "--open_in_browser",
        action="store_true",
        help="open the generated html map in the browser.",
    )
    parser.add_argument(
        "--use_google_maps",
        action="store_true",
        help="use google maps layout instead openstreetmap.",
    )
    parser.add_argument(
        "--enable_clustering",
        type=str,
        choices=["True", "False", "true", "false", "1", "0"],
        default="1",
    )
    parser.add_argument("--output", "-o", help="path to save the generated html map.")
    parser.add_argument(
        "--satellite_view",
        action="store_true",
        help='use satellite view for the map "only available for google map layout".',
    )
    parser.add_argument(
        "--max_zoom",
        type=int,
        default=30,
        help="set the maximum zoom level for the map.",
    )
    parser.add_argument(
        "--zoom_start", type=int, default=15, help="set the zoom level for the map."
    )
    parser.add_argument(
        "--filter", "-f", help='filtering data on map (example "Encryption:OPEN")'
    )
    parser.add_argument(
        "--get_all",
        action="store_true",
        help="search for and return all entries containing the specified substring (case-insensitive) and [it works only with --filter/-f] parm.",
    )
    return parser.parse_args()


def validate_file(wardriving_file, csv_check=True):
    if not os.path.isfile(wardriving_file):
        print(f"{RED}error: the file {wardriving_file} does not exist.{RESET}")
        sys.exit(1)
    if not wardriving_file.lower().endswith(".csv") and csv_check:
        print(f"{RED}error: the file is not a csv file.{RESET}")
        sys.exit(1)


def set_output_file(args, wardriving_file):
    wardriving_html_output = wardriving_file.replace(".csv", ".html")
    if args.output:
        output = args.output.strip()
        if not output:
            print("invalid output name")
            sys.exit(1)
        wardriving_html_output = (
            output if output.endswith(".html") else f"{output}.html"
        )
    return wardriving_html_output


def read_csv_data(wardriving_file):
    try:
        # data = pd.read_csv(wardriving_file, on_bad_lines="skip", na_values=["NaN"])
        data = pd.read_csv(
            wardriving_file,
            encoding="ISO-8859-1",
            on_bad_lines="skip",
            na_values=["NaN"],
        )
    except pd.errors.EmptyDataError:
        print(f'{RED}Error: The file "{wardriving_file}" is empty.{RESET}')
        sys.exit(1)

    except Exception as e:
        print(f"{RED}An unexpected error occurred: {e}{RESET}")
        sys.exit(1)

    columns = list(data.columns)
    if header != columns:
        print(
            f'{RED}error: the csv file has an incorrect header. expected header:\n{header}\n"{wardriving_file}" header:\n{columns}{RESET}'
        )
        sys.exit(1)
    print(f"{GREEN}+ csv file is valid and header matches!{RESET}")
    return data


def create_map(data, args):
    first_ap_coordinates = [data["Latitude"][0], data["Longitude"][0]]
    m = folium.Map(
        location=first_ap_coordinates,
        zoom_start=args.zoom_start,
        max_zoom=args.max_zoom,
    )
    if args.use_google_maps:
        mode = "s" if args.satellite_view else "m"
        folium.TileLayer(
            tiles=f"https://{{s}}.google.com/vt/lyrs={mode}&x={{x}}&y={{y}}&z={{z}}",
            attr="Google",
            name="Google Map Visualizer",
            subdomains=["mt0", "mt1", "mt2", "mt3"],
        ).add_to(m)
    return m


def filter_data(data, args):
    if args.filter:
        filt = args.filter.strip()
        if not filt.__contains__(":"):
            print(f"{RED}Invalid Filter{RESET}")
            sys.exit(1)
        elif filt.lower().startswith("bssid:") or filt.lower().__contains__("time_utc"):
            filt_data = filt.split(":", 1)
            if not (filt_data[1].startswith("[") and filt_data[1].endswith("]")):
                print(
                    f'{RED}For filtering BSSID, use the format: "BSSID:[00:00:00:*]"{RESET}'
                )
                sys.exit(1)
            if len(filt_data[1]) <= 2:
                print(
                    f'{RED}Invalid format. For filtering BSSID, use the format: "BSSID:[00:00:00:*]"{RESET}'
                )
                sys.exit(1)
            filt_data[1] = (
                filt_data[1]
                .replace("[", "")
                .replace("]", "")
                .replace("*", "")
                .replace(":", "", 1)
                .upper()
            )
        else:
            filt_data = filt.split(":")

        if 2 < len(filt_data) and len(filt_data) > 1 or filt_data[1].strip() == "":
            print(f'{RED}Invalid format: "{filt}"{RESET}')
            sys.exit(1)

        # may_contain = []
        may_contain = None
        for index, obj in enumerate(header):
            cond = obj.lower() == filt_data[0].lower()
            if obj.lower() in filt_data[0].lower():
                may_contain = obj
            if cond:
                if filt_data[0].lower() in float_columns:
                    try:
                        float_value = float(filt_data[1])
                        data[obj] = pd.to_numeric(data[obj], errors="coerce")
                        data = data[data[obj].astype(float) == float_value]
                    except ValueError:
                        print(
                            f"{RED}Invalid float value for filtering: {filt_data[1]}{RESET}"
                        )
                        sys.exit(1)
                elif filt_data[0].lower() in int_columns:
                    try:
                        int_value = int(filt_data[1])
                        data[obj] = pd.to_numeric(data[obj], errors="coerce")
                        data = data[data[obj].astype(int) == int_value]
                    except ValueError:
                        print(
                            f"{RED}Invalid int value for filtering: {filt_data[1]}{RESET}"
                        )
                        sys.exit(1)
                else:
                    if args.get_all:
                        if filt_data[0].lower() == "bssid":
                            mac_addrs = filt_data[1]
                            data = data[data[obj].str.startswith(mac_addrs)]
                        else:
                            data = data[
                                data[obj].str.contains(filt_data[1], case=False)
                            ]
                    else:
                        if filt_data[0].lower() == "bssid":
                            mac_addrs = filt_data[1]
                            data = data[data[obj].str.upper() == mac_addrs]
                        else:
                            data = data[data[obj].str.lower() == filt_data[1].lower()]
                if data.__len__() == 0:
                    print(f'{RED}The item "{filt_data[1]}" unfindable{RESET}')
                    sys.exit(1)

                return data
            elif index == len(header) - 1 and not cond:
                print(f'{RED}Invalid header field: "{filt_data[0]}"{RESET}')
                if may_contain:
                    print(f'{BLUE}Did you mean: {RESET}{GREEN}"{may_contain}" ?{RESET}')
                else:
                    print(
                        f"{BLUE}This is available elements for filtering:\n{RESET}{GREEN}{header}{RESET}"
                    )
                sys.exit(1)


def add_markers(data, m, representation_mode):
    coordinates = []
    print(f"{BLUE}Adding markers for {len(data)} entries. {RESET}")

    for idx, row in data.iterrows():
        # Check for valid Latitude and Longitude
        latitude = row["Latitude"]
        longitude = row["Longitude"]
        if pd.isna(latitude) or pd.isna(longitude):
            print(f"{RED}Invalid coordinates for entry {idx}. Skipping...{RESET}")
            continue

        # Check for valid RSSI
        signal_strength = row["RSSI"]
        if pd.isna(signal_strength):
            print(f"{RED}Invalid RSSI value for entry {idx}. Skipping...{RESET}")
            continue

        # Check for valid Accuracy
        accuracy_value = row["Accuracy"]
        if pd.isna(accuracy_value):
            print(
                f"{RED}Invalid Accuracy value for entry {idx}. Setting to NaN.{RESET}"
            )
            accuracy_value = float("nan")  # or set to a default value

        # Determine marker color based on RSSI
        try:
            signal_strength = float(signal_strength)  # Ensure it's a float
            color = (
                "green"
                if signal_strength > -50
                else "orange"
                if signal_strength > -65
                else "red"
            )
        except ValueError:
            print(
                f"{RED}Invalid RSSI value for entry {idx}: {signal_strength}. Skipping...{RESET}"
            )
            continue

        # Prepare popup content
        popup_content = f"""
            <strong>ID:</strong> {idx}<br>
            <strong>Date:</strong> {row["Date"]}<br>
            <strong>Time UTC:</strong> {row["Time_UTC"]}<br>
            <strong>SSID:</strong> {row["SSID"]}<br>
            <strong>BSSID:</strong> {row["BSSID"]}<br>
            <strong>Signal Strength:</strong> {signal_strength} dBm<br>
            <strong>Channel:</strong> {row["Channel"]}<br>
            <strong>Encryption:</strong> {row["Encryption"]}<br>
            <strong>Latitude:</strong> {latitude}<br>
            <strong>Longitude:</strong> {longitude}<br>
            <strong>Altitude:</strong> {row["Altitude"]} meters<br>
            <strong>Satellites:</strong> {row["Satellites"]}<br>
            <strong>Accuracy:</strong> {accuracy_value}<br>
        """

        folium.Marker(
            [latitude, longitude],
            popup=folium.Popup(popup_content, max_width=300),
            tooltip=f"SSID: {row['SSID']}\nBSSID: {row['BSSID']}\nEncryption: {row['Encryption']}",
            icon=folium.Icon(icon="fa-wifi", prefix="fa", color=color),
        ).add_to(representation_mode)
        coordinates.append([latitude, longitude])

    return coordinates


def add_polylines(coordinates, m):
    for i in range(1, len(coordinates)):
        line_color = (
            "lime" if i == 1 else "red" if i == len(coordinates) - 1 else "blue"
        )
        folium.PolyLine(
            [coordinates[i - 1], coordinates[i]],
            color=line_color,
            weight=2.5,
            opacity=1,
        ).add_to(m)


def add_legend(m):
    legend_html = """
    <div style="position: fixed; bottom: 30px; left: 30px; width: 200px; height: 180px;
        background-color: white; border: 2px solid grey; z-index: 9999; font-size: 12px;
        padding: 10px; border-radius: 5px; line-height: 1.4; overflow-y: auto;">
        <strong>Signal Strength (RSSI)</strong><br>
        <i style="background-color: green; width: 15px; height: 15px; display: inline-block;"></i> Strong (-50 dBm or higher)<br>
        <i style="background-color: orange; width: 15px; height: 15px; display: inline-block;"></i> Moderate (-65 dBm)<br>
        <i style="background-color: red; width: 15px; height: 15px; display: inline-block;"></i> Weak (-70 dBm or lower)<br><br>
        <strong>PolyLine Colors</strong><br>
        <i style="background-color: lime; width: 15px; height: 15px; display: inline-block;"></i> First AP<br>
        <i style="background-color: red; width: 15px; height: 15px; display: inline-block;"></i> Last AP<br>
        <i style="background-color: blue; width: 15px; height: 15px; display: inline-block;"></i> Intermediate APs
    </div>
    """
    m.get_root().html.add_child(folium.Element(legend_html))


def save_map(m, wardriving_html_output):
    m.save(wardriving_html_output)
    print(
        f'{GREEN}\n+ Map saved as "{wardriving_html_output}". open it in a browser to view.{RESET}'
    )


def open_in_browser(wardriving_html_output, open_browser):
    if open_browser:
        webbrowser.open(f"file://{os.path.realpath(wardriving_html_output)}")


def main():
    args = parse_arguments()
    wardriving_file = args.csv_file
    validate_file(wardriving_file)
    wardriving_html_output = set_output_file(args, wardriving_file)
    data = read_csv_data(wardriving_file)

    if args.filter:
        data = filter_data(data, args)
        data.reset_index(drop=True, inplace=True)

    if args.print_data.lower().strip() in ["1", "true"]:
        print(f"{BLUE}Csv data:{RESET}")
        print(data)
    if args.print_data.lower() in ["0", "false"]:
        print(f"{BLUE}Csv data:{RESET}\n{data}\n{BLUE}exit{RESET}")
        sys.exit(1)

    m = create_map(data, args)
    representation_mode = (
        MarkerCluster().add_to(m)
        if args.enable_clustering.lower() in ["true", "1"]
        else m
    )
    coordinates = add_markers(data, m, representation_mode)
    add_polylines(coordinates, m)
    add_legend(m)
    save_map(m, wardriving_html_output)
    open_in_browser(wardriving_html_output, args.open_in_browser)


if __name__ == "__main__":
    main()
