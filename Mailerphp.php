#! Swiftmailer with dynamic mail configuration



extract(Config::get('mailtrap-mail'));

	// create new mailer with new settings
	$transport = (new \Swift_SmtpTransport($host, $port))
                       ->setUsername($username)
                       ->setPassword($password)
                       ->setEncryption($encryption);

	\Mail::setSwiftMailer(new \Swift_Mailer($transport));
