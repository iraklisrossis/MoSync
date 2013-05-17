/***************************************************************************
 *
 * This code is based on this article:
 * http://justinangel.net/WindowsPhone7EmulatorAutomation
 *
 * This program uses ICSharpCode.SharpZipLib library to unzip the XAP file
 * You'll need to add that library (freely downloadable) to the project in
 * order to compile it.
 *
 * https://github.com/icsharpcode/SharpZipLib
 *
 * This program was modified by MoSync in 2013.
 *
 * Original version:
 * http://wpapplauncher.codeplex.com/
 *
 * WP7 App Launcher
 * Copyright (C) 2011 anderZubi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *********************************************************************************/

using System;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
//using Microsoft.SmartDevice.Connectivity;
using System.GAC;

namespace WPAppLauncher
{
	class Program
	{
		private static void ShowHelp()
		{
			Console.WriteLine("WPAppLauncher.exe < /dump | [/wait:<filename>] <platform> <target> <yourXapFile.xap> >");
			Console.WriteLine("");
			Console.WriteLine("\t/dump prints a complete list of available platforms and targets, then exits.");
			Console.WriteLine("\t/wait causes the launcher to wait until the specified file exists.");
			Console.WriteLine("\t<yourXapFile.xap> is the XAP file you want to launch.");
		}

		static string sPlatform;
		static string sTarget;
		static string waitFile = null;

		static string xapFile;

		static void Main(string[] args)
		{
#if !DEBUG
			try
#endif
			{
				if (args.Length == 1 && args[0] == "/dump")
					dumpAssemblyInfo();
				else if (args.Length >= 3)
					ProcessOptions(args);
				else
					ShowHelp();
			}
#if !DEBUG
			catch (System.Reflection.TargetInvocationException tie)
			{
				Console.WriteLine("Exception: " + tie.InnerException.Message);
				Environment.Exit(1);
			}
			catch (Exception e)
			{
				Console.WriteLine("Exception: " + e.Message);
				Environment.Exit(1);
			}
#endif
		}

		private static void ProcessOptions(string[] args)
		{
			for (int i = 0; i < args.Length - 3; i++)
			{
				if (args[i].StartsWith("/wait:"))
					waitFile = args[i].Substring(6);
				else
				{
					throw new Exception("Exception: option not recognized: " + args[i]);
				}
			}
			sPlatform = args[args.Length - 3];
			sTarget = args[args.Length - 2];
			xapFile = args[args.Length - 1];
			if (!File.Exists(xapFile))
			{
				throw new Exception("File '" + xapFile + "' does not exist!");
			}
			LaunchApp();
		}

		private static void dumpAssemblyInfo()
		{
			var assemblies = new System.Collections.ArrayList();
			Console.WriteLine("Enumerating versions of Microsoft.SmartDevice.Connectivity...");
			var e = AssemblyCache.CreateGACEnum("Microsoft.SmartDevice.Connectivity");
			while (true)
			{
				IAssemblyName an;
				e.GetNextAssembly(new IntPtr(0), out an, 0);
				if (an == null)
					break;
#if false
				uint vh, vl;
				an.GetVersion(out vh, out vl);
				Console.WriteLine("v" + vh + "." + vl);
#endif
				uint len = 1000;
				var dnb = new System.Text.StringBuilder(1000);
				an.GetDisplayName(dnb, ref len, 0);
				string dn = dnb.ToString();
				Console.WriteLine(dn);
				var a = System.Reflection.Assembly.Load(new System.Reflection.AssemblyName(dn));
				assemblies.Add(a);

				Type t = a.GetType("Microsoft.SmartDevice.Connectivity.DatastoreManager");
				Type[] constructorTypes = { typeof(int) };
				if (t == null)
				{
					Console.WriteLine("Module loaded. Available types:");
					foreach (var at in a.GetTypes())
					{
						Console.WriteLine(at.FullName);
					}
				}

				object[] parameters = { 1033 };
				object dm = t.GetConstructor(constructorTypes).Invoke(parameters);

				var platforms = (System.Collections.ICollection)invoke(dm, "GetPlatforms");
				Console.WriteLine("Available platforms/devices:");
				foreach (var p in platforms)
				{
					var name = gpv(p, "Name");
					Console.WriteLine(name);

					var devices = (System.Collections.ICollection)invoke(p, "GetDevices");
					foreach (var d in devices)
					{
						var devName = gpv(d, "Name");
						Console.WriteLine("\t" + devName);
					}
				}
				Console.WriteLine("");
			}
		}

		// get property value
		private static object gpv(object o, string propName)
		{
			return o.GetType().GetProperty(propName).GetValue(o, null);
		}

		private static object invoke(object o, string funcName, params object[] parameters)
		{
			var t = o.GetType();
			System.Reflection.MethodInfo m;
			if(parameters == null || parameters.Length == 0)
				m = t.GetMethod(funcName, new Type[0]);
			else
				m = t.GetMethod(funcName);
			return m.Invoke(o, parameters);
		}

		private static object GetDevice(string platName, string devName)
		{
			Console.WriteLine("Looking for '" + platName + "'/'" + devName + "'...");
			// iterate over assemblies with this name.
			var e = AssemblyCache.CreateGACEnum("Microsoft.SmartDevice.Connectivity");
			while (true)
			{
				// load next assembly, if there is one.
				IAssemblyName an;
				e.GetNextAssembly(new IntPtr(0), out an, 0);
				if (an == null)
					break;
				uint len = 1000;
				var dnb = new System.Text.StringBuilder(1000);
				an.GetDisplayName(dnb, ref len, 0);
				string assemblyName = dnb.ToString();
				var a = System.Reflection.Assembly.Load(new System.Reflection.AssemblyName(assemblyName));

				// construct DatastoreManager.
				Type t = a.GetType("Microsoft.SmartDevice.Connectivity.DatastoreManager");
				Type[] constructorTypes = { typeof(int) };
				object[] parameters = { 1033 };
				object dm = t.GetConstructor(constructorTypes).Invoke(parameters);

				// get collection of platforms.
				var platforms = (System.Collections.ICollection)invoke(dm, "GetPlatforms");
				foreach (var p in platforms)
				{
					if ((string)gpv(p, "Name") != platName)
						continue;

					var devices = (System.Collections.ICollection)invoke(p, "GetDevices");
					foreach (var d in devices)
					{
						if ((string)gpv(d, "Name") == devName)
						{
							Console.WriteLine("Found it in " + assemblyName);
							return d;
						}
					}
				}
			}
			Console.WriteLine("'" + platName + "'/'" + devName + "' not found. Available targets:");
			dumpAssemblyInfo();
			throw new Exception();
		}

		private static void LaunchApp()
		{
			var device = GetDevice(sPlatform, sTarget);

			// Get AppID.
			var xipFile = new ICSharpCode.SharpZipLib.Zip.ZipFile(xapFile);
			Guid appID = GetAppID(xipFile.GetInputStream(xipFile.GetEntry(("WMAppManifest.xml"))));

			// Connect to Emulator / Device
			Console.WriteLine("Connecting to '" + sTarget + "'...");
			invoke(device, "Connect");
			Console.WriteLine(device + " Connected.");

			// Remove old version, if installed.
			object app = null;
			if ((Boolean)invoke(device, "IsApplicationInstalled", appID))
			{
				Console.WriteLine("Uninstalling XAP...");
				app = invoke(device, "GetApplication", appID);
				invoke(app, "Uninstall");
				Console.WriteLine("XAP Uninstalled...");
			}

			// Extract icon.
			Console.WriteLine("Extracting application icon...");
			string extractionFolder = Path.GetTempPath() + Path.DirectorySeparatorChar + Path.GetRandomFileName();
			Directory.CreateDirectory(extractionFolder);
			string iconPath = extractionFolder + Path.DirectorySeparatorChar + "ApplicationIcon.png";
			var iconFile = new FileStream(iconPath, FileMode.Create);
			xipFile.GetInputStream(xipFile.GetEntry("ApplicationIcon.png")).CopyTo(iconFile);
			iconFile.Close();
			Console.WriteLine("Extracted application icon.");

			// Install XAP.
			Console.WriteLine("Installing XAP...");
			app = invoke(device, "InstallApplication", appID, appID, "NormalApp", iconPath, xapFile);
			Console.WriteLine("XAP installed.");

			// Launch Application.
			Console.WriteLine("Launching app...");
			invoke(app, "Launch");
			Console.WriteLine("Launched app.");

			// Delete icon.
			Console.WriteLine("Deleting application icon...");
			Directory.Delete(extractionFolder, true);
			Console.WriteLine("Deleted application icon.");

			if (waitFile != null)
			{
				Console.WriteLine("Waiting for the app to create '" + waitFile + "'...");
				while (true)
				{
					try
					{
						var store = invoke(app, "GetIsolatedStore");
						invoke(store, "ReceiveFile", waitFile, waitFile);
						break;
					}
					catch (System.IO.FileNotFoundException)
					{
						System.Threading.Thread.Sleep(1000);
					}
				}
				Console.WriteLine(waitFile + " retrieved.");
			}
		}

		static Guid GetAppID(Stream appManifestStream)
		{
			Guid guid = new Guid();

			using (StreamReader sReader = new StreamReader(appManifestStream))
			{
				Regex regex = new Regex(@"(\{{0,1}([0-9a-fA-F]){8}-([0-9a-fA-F]){4}-([0-9a-fA-F]){4}-([0-9a-fA-F]){4}-([0-9a-fA-F]){12}\}{0,1})");

				bool idFound = false;
				string currentLine = sReader.ReadLine();


				while (!idFound && currentLine != null)
				{
					Match m = regex.Match(currentLine);
					if (m.Success)
					{
						idFound = true;
						Guid.TryParse(m.Value, out guid);
						break;
					}
					currentLine = sReader.ReadLine();
				}
			}

			Console.WriteLine("AppID: " + guid.ToString());
			return guid;
		}
	}
}
