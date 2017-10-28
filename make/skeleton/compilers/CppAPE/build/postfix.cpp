#ifdef CPPAPE_PROCESSOR_H

template<class ProcessorType>
int registerClass(ProcessorType *)
{
	FactoryBase::SetCreater(&ProcessorFactory<ProcessorType>::create);
	return 0;
}

#endif