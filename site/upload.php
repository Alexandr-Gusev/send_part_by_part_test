<?

if (is_uploaded_file($_FILES['userfile']['tmp_name']))
{
	if (move_uploaded_file($_FILES['userfile']['tmp_name'], 'uploads/'.basename($_FILES['userfile']['name'])))
	{
		print 'file ok';
	}
}

?>