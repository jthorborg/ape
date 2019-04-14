
#ifdef CPPAPE_PROCESSOR_H

template<class ProcessorType>
int registerClass(ProcessorType *)
{
	ape::FactoryBase::SetCreater(&ape::ProcessorFactory<ProcessorType>::create);
	return 0;
}

#endif