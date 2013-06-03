using System;
using System.IO;
using System.Collections.Generic;
using System.Windows.Resources;
using System.Windows;

// This is the core that interprets mosync
// byte code (produced by pipe-tool).
// It has dependencies on some idl compiler
// generated code, such as the SyscallInvoker

namespace MoSync
{

  public class CoreNative : Core
  {
		public struct ComplexInt
		{
			public int r, i;
		}

		public struct ComplexDouble
		{
			public double r, i;
		}

				protected Syscalls mSyscalls;
        protected int sp;

        public CoreNative()
        {
        }

        public void InitData(String dataName, int fileSize, int dataSegmentSize)
        {
            mDataMemory = new Memory(dataSegmentSize);

            StreamResourceInfo dataSectionInfo = Application.GetResourceStream(new Uri("RebuildData\\" + dataName, UriKind.Relative));

            if (dataSectionInfo == null || dataSectionInfo.Stream == null)
            {
                MoSync.Util.CriticalError("No data_section.bin file available!");
            }

            Stream dataSection = dataSectionInfo.Stream;
            mDataMemory.WriteFromStream(0, dataSection, fileSize);
            dataSection.Close();

						sp = dataSegmentSize - 16;
            int customEventDataSize = 60;
            sp -= customEventDataSize;
            mCustomEventPointer = dataSegmentSize - customEventDataSize;
        }


        public override void Init()
        {
            base.Init();
            Start();

            if (mRuntime == null)
                MoSync.Util.CriticalError("No runtime!");

            mSyscalls = mRuntime.GetSyscalls();
        }

				public override int GetStackPointer()
        {
            return sp;
        }

        public void MoSyncDiv0()
        {
            MoSync.Util.CriticalError("Division by zero!");
        }

        public virtual void Main()
        {
        }

        public override void Run()
        {
            Main();
        }

			protected const int zr = 0;

			protected int RINT(int address)
			{
				return mDataMemory.ReadInt32(address);
			}
			protected void WINT(int address, int data)
			{
				mDataMemory.WriteInt32(address, data);
			}

			protected byte RBYTE(int address)
			{
				return mDataMemory.ReadUInt8(address);
			}
			protected void WBYTE(int address, int data)
			{
				mDataMemory.WriteUInt8(address, (byte)data);
			}

			protected ushort RSHORT(int address)
			{
				return mDataMemory.ReadUInt16(address);
			}
			protected void WSHORT(int address, int data)
			{
				mDataMemory.WriteUInt16(address, (ushort)data);
			}

			protected void WDOUBLE(int address, double data)
			{
				mDataMemory.WriteDouble(address, data);
			}

			protected void WFLOAT(int address, double data)
			{
				mDataMemory.WriteFloat(address, (float)data);
			}

			protected void MOV_DIDF(int i0, int i1, out double d)
			{
				d = MoSync.Util.ConvertToDouble(i0, i1);
			}

			protected void MOV_DFDI(out int i0, out int i1, double d)
			{
				MOV_DI(out i0, out i1, BitConverter.DoubleToInt64Bits(d));
			}

			protected void MOV_DI(out int i0, out int i1, long value)
			{
				i0 = (int)(value & 0xffffffff);
				i1 = (int)(((ulong)value) >> 32);
			}

			protected long RETURN_DI(int i0, int i1)
			{
				ulong l = ((ulong)(uint)i0) | (((ulong)(uint)i1) << 32);
				return (long)l;
			}

			protected void MOV_SISF(int i0, out double d)
			{
				d = MoSync.Util.ConvertToFloat(i0);
			}

			protected void MOV_SFSI(out int i0, double d)
			{
				i0 = MoSync.Util.ConvertToInt((float)d);
			}

			protected void FLOATS_DIDF(int i0, int i1, out double d)
			{
				d = (double)RETURN_DI(i0, i1);
			}

			protected void FLOATU_DIDF(int i0, int i1, out double d)
			{
				d = (double)(ulong)RETURN_DI(i0, i1);
			}

			protected void FIXS_DFDI(out int i0, out int i1, double d)
			{
				MOV_DI(out i0, out i1, (long)d);
			}

			protected void FIXU_DFDI(out int i0, out int i1, double d)
			{
				ulong u;
				if (d > long.MaxValue)
					// This conversion loses the last 4 digits of precision.
					// But it's better than the direct conversion from double to ulong,
					// which, for this range, returns zero.
					u = (ulong)(decimal)d;
				else
					u = (ulong)d;
				MOV_DI(out i0, out i1, (long)u);
			}

			protected ComplexDouble RETURN_CF(double d0, double d1)
			{
				ComplexDouble cd;
				cd.r = d0;
				cd.i = d1;
				return cd;
			}

			protected ComplexInt RETURN_CI(int i0, int i1)
			{
				ComplexInt ci;
				ci.r = i0;
				ci.i = i1;
				return ci;
			}

			protected void MOV_CF(out double d0, out double d1, ComplexDouble cd)
			{
				d0 = cd.r;
				d1 = cd.i;
			}

			protected void MOV_CI(out int i0, out int i1, ComplexInt ci)
			{
				i0 = ci.r;
				i1 = ci.i;
			}

			protected double sqrt(double d)
			{
				return Math.Sqrt(d);
			}

			protected double sin(double d)
			{
				return Math.Sin(d);
			}

			protected double cos(double d)
			{
				return Math.Cos(d);
			}

			protected double exp(double d)
			{
				return Math.Exp(d);
			}

			protected double log(double d)
			{
				return Math.Log(d);
			}

			protected double pow(double a, double b)
			{
				return Math.Pow(a, b);
			}

			protected double atan2(double a, double b)
			{
				return Math.Atan2(a, b);
			}

			protected void maPanic(int code, String message)
			{
				MoSync.Util.CriticalError(message + "\ncode: " + code);
			}
		}
}
