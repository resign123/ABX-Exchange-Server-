# ABX Exchange Assessment

This project consists of an exchange server and a client for packet transmission. Follow the steps below to set up and run the application on your local machine.

## Prerequisites

- Node.js installed on your machine.
- g++ compiler installed (for compiling the C++ client).
- Git (optional, for cloning the repository).

## Instructions to Run the Code

1. **Download the Repository:**
   - Go to the repository on GitHub.
   - Click on the green "Code" button.
   - Select "Download ZIP" and save the file to your machine.

2. **Extract the ZIP File:**
   - Navigate to the folder where you downloaded the ZIP file.
   - Right-click on the ZIP file and select "Extract All" to a folder of your choice.

3. **Open the Project in Visual Studio Code:**
   - Launch Visual Studio Code.
   - Open the folder where you extracted the files by selecting `File` → `Open Folder...`.

4. **Run the Server:**
   - Open the terminal in Visual Studio Code.
   - Change directory to the server folder:
     ```bash
     cd path_to_your_extracted_folder/abx_exchange_server
     ```
   - Start the server by executing:
     ```bash
     node main.js
     ```

5. **Open a New Terminal for the Client:**
   - In Visual Studio Code, open a new terminal by selecting `Terminal` → `New Terminal`.
   - Change directory back to the parent folder:
     ```bash
     cd path_to_your_extracted_folder
     ```
   - Then navigate to the client folder:
     ```bash
     cd abx_client
     ```

6. **Compile the Client:**
   - Use the following command to compile the C++ client:
     ```bash
     g++ -o abx_client client.cpp -I"json-develop/single_include" -lws2_32
     ```

7. **Run the Client:**
   - After compilation, execute the client with the following command:
     ```bash
     ./abx_client.exe
     ```

8. **Check Output:**
   - The output received will be stored in `output.json`, containing all sequences.
   - In the terminal, you should see output similar to:
     ```
     Received packet: 1
     Received packet: 2
     Received packet: 4
     Received packet: 5
     Received packet: 8
     Received packet: 9
     Received packet: 10
     Received packet: 11
     Received packet: 14
     Error receiving packet or connection closed
     Requesting 5 missing packets...
     Received packet: 3
     Received packet: 6
     Received packet: 7
     Received packet: 12
     Received packet: 13
     Data received and written to output.json
     Total packets received: 14
     ```

## Conclusion

Following these steps, you should be able to run the ABX Exchange successfully. If you encounter any issues, please ensure that all prerequisites are met and that you are in the correct directory while executing commands.
