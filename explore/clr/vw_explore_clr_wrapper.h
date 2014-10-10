#pragma once

#include "mwt.h"

#include <msclr\marshal_cppstd.h>

using namespace System;
using namespace System::Runtime::InteropServices;

namespace MultiWorldTesting {

	[StructLayout(LayoutKind::Sequential)]
	public value struct FEATURE
	{
		float X;
		UInt32 WeightIndex;
	};

	public ref class CONTEXT
	{
	public:
		CONTEXT(cli::array<FEATURE>^ features, String^ otherContext)
		{
			Features = features;
			OtherContext = otherContext;
		}
	public:
		cli::array<FEATURE>^ Features;
		String^ OtherContext;
	};

	public ref class INTERACTION
	{
	public:
		CONTEXT^ ApplicationContext;
		UInt32 ChosenAction;
		float Probability;
		UInt64 JoinId;
	};

	public ref class ActionID
	{
	public:
		static UInt32 Make_OneBased(UInt32 id) { return MWTAction::Make_OneBased(id); }
		static UInt32 Make_ZeroBased(UInt32 id) { return MWTAction::Make_ZeroBased(id); }
	};

	generic <class T>
	public delegate UInt32 StatefulPolicyDelegate(T, CONTEXT^);
	public delegate UInt32 StatelessPolicyDelegate(CONTEXT^);

	generic <class T>
	public delegate void StatefulScorerDelegate(T, CONTEXT^, cli::array<float>^ scores);
	public delegate void StatelessScorerDelegate(CONTEXT^, cli::array<float>^ scores);

	// Internal delegate denifition
	private delegate UInt32 InternalStatefulPolicyDelegate(IntPtr, IntPtr);
	private delegate void InternalStatefulScorerDelegate(IntPtr, IntPtr, IntPtr scores, UInt32 size);

	interface class IFunctionWrapper
	{
		public:
			virtual UInt32 InvokeFunction(CONTEXT^) abstract;
			virtual void InvokeScorer(CONTEXT^, cli::array<float>^) abstract;
	};

	generic <class T>
	public ref class DefaultPolicyWrapper : IFunctionWrapper
	{
		public:
			DefaultPolicyWrapper(StatefulPolicyDelegate<T>^ policyFunc, T policyParams)
			{
				defaultPolicy = policyFunc;
				parameters = policyParams;
			}

			DefaultPolicyWrapper(StatefulScorerDelegate<T>^ scorerFunc, T policyParams)
			{
				defaultScorer = scorerFunc;
				parameters = policyParams;
			}

			DefaultPolicyWrapper(StatelessPolicyDelegate^ policyFunc)
			{
				statelessPolicy = policyFunc;
			}

			DefaultPolicyWrapper(StatelessScorerDelegate^ scorerFunc)
			{
				statelessScorer = scorerFunc;
			}

			virtual UInt32 InvokeFunction(CONTEXT^ c) override
			{
				if (defaultPolicy != nullptr)
				{
					return defaultPolicy(parameters, c);
				}
				else
				{
					return statelessPolicy(c);
				}
			}

			virtual void InvokeScorer(CONTEXT^ c, cli::array<float>^ scores) override
			{
				if (defaultScorer != nullptr)
				{
					defaultScorer(parameters, c, scores);
				}
				else
				{
					statelessScorer(c, scores);
				}
			}
		private:
			T parameters;
			StatefulPolicyDelegate<T>^ defaultPolicy;
			StatelessPolicyDelegate^ statelessPolicy;
			StatefulScorerDelegate<T>^ defaultScorer;
			StatelessScorerDelegate^ statelessScorer;
	};

	public ref class MwtExplorer
	{
	private:
		MWTExplorer* m_mwt;
		IFunctionWrapper^ policyWrapper;
		cli::array<IFunctionWrapper^>^ policyWrappers;
		GCHandle selfHandle;
		cli::array<IntPtr>^ baggingParameters;

	public:
		MwtExplorer();
		~MwtExplorer();

		generic <class T>
		void InitializeEpsilonGreedy(float epsilon, StatefulPolicyDelegate<T>^ defaultPolicyFunc, T defaultPolicyFuncParams, UInt32 numActions);
		void InitializeEpsilonGreedy(float epsilon, StatelessPolicyDelegate^ defaultPolicyFunc, UInt32 numActions);

		generic <class T>
		void InitializeTauFirst(UInt32 tau, StatefulPolicyDelegate<T>^ defaultPolicyFunc, T defaultPolicyFuncParams, UInt32 numActions);
		void InitializeTauFirst(UInt32 tau, StatelessPolicyDelegate^ defaultPolicyFunc, UInt32 numActions);

		generic <class T>
		void InitializeBagging(UInt32 bags, cli::array<StatefulPolicyDelegate<T>^>^ defaultPolicyFuncs, cli::array<T>^ defaultPolicyArgs, UInt32 numActions);
		void InitializeBagging(UInt32 bags, cli::array<StatelessPolicyDelegate^>^ defaultPolicyFuncs, UInt32 numActions);

		generic <class T>
		void InitializeSoftmax(float lambda, StatefulScorerDelegate<T>^ defaultScorerFunc, T defaultScorerFuncParams, UInt32 numActions);
		void InitializeSoftmax(float lambda, StatelessScorerDelegate^ defaultScorerFunc, UInt32 numActions);

		void Unintialize();

		UInt32 ChooseAction(CONTEXT^ context, String^ uniqueId);
		Tuple<UInt32, UInt64>^ ChooseActionAndKey(CONTEXT^ context);

		String^ GetAllInteractionsAsString();
		cli::array<INTERACTION^>^ GetAllInteractions();

	// Helper methods
	public:
		static cli::array<float>^ IntPtrToScoreArray(IntPtr scoresPtr, UInt32 size);

	internal:
		UInt32 InvokeDefaultPolicyFunction(CONTEXT^);
		UInt32 InvokeBaggingDefaultPolicyFunction(CONTEXT^, int);
		void InvokeDefaultScorerFunction(CONTEXT^, cli::array<float>^);

	private: // Internal Initialize APIs
		void InitializeEpsilonGreedy(float epsilon, InternalStatefulPolicyDelegate^ defaultPolicyFunc, IntPtr defaultPolicyFuncContext, UInt32 numActions);
		void InitializeTauFirst(UInt32 tau, InternalStatefulPolicyDelegate^ defaultPolicyFunc, IntPtr defaultPolicyFuncContext, UInt32 numActions);
		void InitializeBagging(UInt32 bags, cli::array<InternalStatefulPolicyDelegate^>^ defaultPolicyFuncs, cli::array<IntPtr>^ defaultPolicyArgs, UInt32 numActions);
		void InitializeSoftmax(float lambda, InternalStatefulScorerDelegate^ defaultScorerFunc, IntPtr defaultPolicyFuncContext, UInt32 numActions);

	private: // Internal Callback Methods
		static UInt32 InternalStatefulPolicy(IntPtr, IntPtr);
		static UInt32 BaggingStatefulPolicy(IntPtr, IntPtr);
		static void InternalScorerFunction(IntPtr, IntPtr, IntPtr, UInt32);
	};

	private value struct BaggingParameter
	{
	public:
		MwtExplorer^ Mwt;
		int BagIndex;
	};
}
