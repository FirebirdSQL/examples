#include <exception>
#include <iostream>
#include "firebird/Interface.h"


namespace fb = Firebird;


int main()
{
	const auto master = fb::fb_get_master_interface();
	const auto util = master->getUtilInterface();
	const auto status = master->getStatus();
	fb::ThrowStatusWrapper statusWrapper(status);

	try
	{
		const auto provider = master->getDispatcher();

		const auto dpb = util->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::DPB, nullptr, 0);
		dpb->insertString(&statusWrapper, isc_dpb_user_name, "sysdba");
		dpb->insertString(&statusWrapper, isc_dpb_password, "masterkey");

		const auto attachment = provider->createDatabase(&statusWrapper, "firebird-example-test.fdb",
			dpb->getBufferLength(&statusWrapper), dpb->getBuffer(&statusWrapper));

		std::cout << "Database created" << std::endl;

		attachment->dropDatabase(&statusWrapper);

		std::cout << "Database dropped" << std::endl;

		dpb->dispose();
		attachment->release();
		provider->release();
		status->dispose();
	}
	catch (const fb::FbException& e)
	{
		char buffer[1024];
		util->formatStatus(buffer, sizeof(buffer), e.getStatus());
		std::cerr << "Firebird error: " << buffer << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Generic error: " << e.what() << std::endl;
	}

	return 0;
}
