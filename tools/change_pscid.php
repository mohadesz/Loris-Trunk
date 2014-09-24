<?php
/**
 * Changes the pscid
 *
 * PHP version 5
 *
 * @category Utility_Script
 * @package  Loris_Script 
 * @author   Zia Mohaddes  <zia.mohades@gmail.com>
 * @license  Loris License
 * @link     https://github.com/aces/IBIS
 */
require_once "generic_includes.php";

/**
 * User prompt
 */
print "count is " . count($argv);
if ((count($argv)!=5)) {

    echo "Usage: php change_pscid.php -o Original_PSCID -n New_PSCID\n";
    echo "Example: php change_pscid.php -o 14-44-04 -n 14-43-04 \n";

    die();
}

/**
 * parse the options
 */
$opts = getopt("o:n:");
$current_pscid = null;
$new_pscid = null;
if (!is_array($opts)) {
    print "There was a problem reading in the options.\n\n";
    exit(1);
}

/**
 * for current PSCID
 */
if ($opts['o']!=null) {
    $current_pscid = $opts['o'];
}

/**
 * for new PSCID
 */
if ($opts['n']!=null) {
    $new_pscid = $opts['n'];
}


print "current pscid : $current_pscid new pscid : $new_pscid";
//get the current candidate
$current_candidate= $DB->pselectOne(
    "SELECT COUNT(*) FROM candidate WHERE ".
    "PSCID =:pid",
array('pid'=>$current_pscid)
);
if ($current_candidate==0) {
    die("The current candidate: ".
    $current_candidate ." doesnt exist");
}
//make sure the new candidate doesn't already exist
$current_candidate= $DB->pselectOne(
    "SELECT COUNT(*) FROM candidate WHERE PSCID =:newpid",
    array('newpid'=>$new_pscid)
);
/*
if ($current_candidate==0) {
    die("The new pscid already exists");
}
*/

///////////////////////////////////////////////////////////////////////////////
//////////////////////////Delete wrongly created candidate/////////////////////
///////////////////////////////////////////////////////////////////////////////

$DB->delete('candidate',array('PSCID'=>'14_43_04'));

///////////////////////////////////////////////////////////////////////////////
/////////////////////change the commentID//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///get the current commentid
$info = $DB->pselect(
    "SELECT f.CommentID,f.Test_name FROM candidate c".
    " JOIN session s ON (s.candid=c.candid)".
    " JOIN flag f ON (f.sessionid=s.ID)".
    " WHERE c.pscid = :pid",
    array('pid'=>$current_pscid)
);


foreach ($info as $data) {
    print_r($data);
    if ((isset($data))) {
        $commentid = $data['CommentID'];
        $test_name = $data['Test_name'];
        ///////////////////////////////////////////////////////////////////////
        /////////////////////construct the new commentid///////////////////////
        ///////////////////////////////////////////////////////////////////////
        if (substr($commentid,0,3) =='DDE') {
            $new_commentid = str_replace(
                substr($commentid,10,8),
                $new_pscid,$commentid
            );
        } else {
            $new_commentid = str_replace(
                substr($commentid,6,8),
                $new_pscid,$commentid
            );
        }
        

        ////////////////////////////////////////////////////////////////////////
        ///////////////////////update flag with new commentid///////////////////
        ////////////////////////////////////////////////////////////////////////
        $DB->update(
            'flag',
            array('CommentID'=>$new_commentid),
            array('CommentID'=>$commentid)
        );

        /////////////////////////////////////////////////////////////////////////
        ////modify the instrument table//////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////
        $DB->update(
            $test_name,
            array('CommentID'=>$new_commentid),
            array('CommentID'=>$commentid)
        );
    }
}


///////////////////////////////////////////////////////////////////////////////
///////////////////modify the candidate table//////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

$DB->update(
    'candidate',
     array('PSCID'=>$new_pscid),array('PSCID'=>$current_pscid)
);

?>
