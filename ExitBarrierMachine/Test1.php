<?php
include "dbconn.inc"

$conf = parse_ini_file('config.ini');
$TicketNo = $_GET[TicketNo];
global $connOld;

if(strcmp(substr($TicketNo,1),$conf["ValidTicketNo"]) == 0)
{
	$dbsqlExitJournal = "SELECT TICKET_NO, DT_IN, DT_OUT FROM exitjournal where TICKETNO=$'TicketNo'";

	$dbresultCpReceipt = ibase_query($connOld,$dbsqlCpReceipt) or die('dbsearch: '.ibase_errmsg()." | ".$dbsqlCpReceipt);
	$rowCpReceipt = ibase_fetch_row($dbresultCpReceipt);
	$dbresultExitJournal = ibase_query($connOld,$dbsqlExitJournal) or die('dbsearch: '.ibase_errmsg()." | ".$dbsqlExitJournal);
	$rowExitJournal = ibase_fetch_row($dbresultExitJournal);

	if (is_null($rowExitJournal[0]))
	{
	$dbsqlCpReceipt = "SELECT TICKET_NO, DT_IN, DT_OUT FROM cp_receipt where TICKET_NO=$'TicketNo'";
		if (is_null($rowCpReceipt[0])) 
		{
			//return either empty or error
			echo "Empty or Error"
		}
		else
		{
			// output data of each row
			echo "id: " . $rowCpReceipt[0]. " DT_IN " . $rowCpReceipt[1]. " DT_OUT " . $rowCpReceipt[2];
			$date = $rowCpReceipt[1];
			date_default_timezone_set("Asia/Kuala_Lumpur");
			$dateTocmp = new DateTime();

			$secs = date_timestamp_get($dateTocmp) - strtotime($datetime1);// == <seconds between the two times>
				
			if($secs > (900))
			{
				echo "Expired";
			}
			else
			{
				echo "OK";
			}
		}
	}
	else
	{
		echo "Anti-Passback";
	}
}
else
{
	echo "Invalid Ticket";
}
?>