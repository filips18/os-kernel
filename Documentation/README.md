# How to run this project with Borland C 3.1
These are the required steps, the IDE used is Eclipse:
  1. Start Eclipse, choose a workspace.
  2. Create a new C++ project.
  3. While in the naming window, name your project, click on "Makefile project", select "Empty Project" and in the right window select "-- Other Toolchain --". Click Next.
  4. Click "Advanced settings".Choose the option "C/C++ Build" -> "Tool Chain Editor".Uncheck the option "Display compatible toolchains only".For the option "Current toolchain" select "bccToolchain", and for the option "Current builder" select "bcc".
  5. In the same window, choose the option "C/C++ Build".Check the option "Generate Makefiles Automatically" in the right window.
  6. In the same window, choose the option "C/C++ General" -> "Preprocessor include paths, macros, etc." and click on the tab "Providers" in the right window.Check the option "bcc_specific".While in the same window, switch to the tab "Entries".Click on the button "Add..".Choose the option "Include Directory" from the first dropdown list, and the option "File System Path" from the second dropdown list.In the "Path" box, copy the path to the folder "INCLUDE" from the extracted archive from the folder "Borland C 3.1" of this repository.Check the options "Treat as built-in" and "Contains system headers".
  7. Click "Finish", Eclipse will create a new project with these settings.
  8. Right click on the project in Project Explorer, and select the option "BCC" -> "Select bcc directory". Insert the path to the extracted bc31 archive from the "Borland C 3.1" folder of this repository.Also from the same option select "BCC" -> "Select memory model" -> "Huge".
  
  Right clicking the project, and selecting "BCC" will generate a number of options, these are the options used to work with this project.
