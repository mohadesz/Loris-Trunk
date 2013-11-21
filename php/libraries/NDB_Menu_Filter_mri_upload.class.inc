<?php

/**
 * mri_upload class file
 *
 * PHP version 5
 * 
 * @category Behavioural
 * @package  Main 
 * @author   Zia Mohaddes  <zia.mohades@gmail.com>
 * @license  Loris License
 * @link     https://github.com/mohadesz/Loris-Trunk
*/

require_once 'NDB_Menu_Filter_Form.class.inc';
require_once 'Log.class.inc';


/**
 * Log Class
 *
 * This class is MRI_upload Filter Menu
 * 
 * @category Behavioural
 * @package  Main 
 * @author   Zia Mohaddes  <zia.mohades@gmail.com>
 * @license  Loris License
 * @link     https://github.com/mohadesz/Loris-Trunk
*/
class NDB_Menu_Filter_Mri_Upload extends NDB_Menu_Filter_Form
{
    //var $log;
    /**
    * overloading this method to allow access to site users 
    * (their own site only) and users w/ multisite privs
    * 
    * @note   overloaded function
    * 
    * @return bool
    * @access private
    */

    function _hasAccess()
    {
        // create user object
        $user =& User::singleton();
        if (Utility::isErrorX($user)) {
            return PEAR::raiseError("User Error: ".$user->getMessage());
        }
        return true;
    }

    /**
     * Sets up the Filter Variables
     *
     * @return boolean
     */
    function _setupVariables()
    {

        // set the class variables

        $this->columns = array('UploadID','s.CandID','s.visit_label',
        'SourceLocation','FileType','UploadDate','UploadedBy',
        'TarchiveID','MincInserted AS MincData' );

        $this->validFilters = array('SourceLocation');
        $this->query = " FROM mri_upload mu
           JOIN session s ON (s.ID = mu.SessionID)
            WHERE 1=1 ";
        $this->formToFilter = array(
        'SourceLocation' => 'SourceLocation'
        );

        $this->log= new Log("MRI_upload", "/data/gusto/data/logs/");



        return true;
    }

    /**
     * Sets up the Filter Form
     *
     * @return boolean
     */
    function _setFilterForm()
    {

        // create user object
        /////////////////////////////////////////////////////////////
        ////////////////////////Upload related fields///////////////
        /////////////////////////////////////////////////////////////
        $this->form->addElement('file', 'mri_file', 'File to Upload');
        $this->form->addElement('static', 'File_name', 'File name');
        $this->form->addElement('static', 'status', 'Status');

        /////////////////////////////////////////////////////////////
        //////////////////Patient-name and Patient-ID fields/////////
        /////////////////////////////////////////////////////////////
        $this->addBasicText('CandID', 'CandID:');
        $this->addBasicText('PSCID', 'PSCID:');
        $visitlabels = Utility::getAllVisits();
        $this->addSelect('VisitLabel', 'VisitLabel:', $visitlabels);
        $new_files = array();
        $DB =& Database::singleton();

        $files = $DB->pselect(
            'SELECT distinct SourceLocation FROM mri_upload',
            array()
        );

        foreach ($files as $file) {
            $new_files[$file['SourceLocation']] =$file['SourceLocation'];
        }
        $new_files = array(null=>'Any') + $new_files;
        $this->form->addElement(
            'select', 'SourceLocation', 
            'SourceLocation', $new_files
        );
        
        if (empty($_FILES) && empty($_POST)  
            && ($_SERVER['REQUEST_METHOD']=='POST')
        ) { //catch file overload error...
            //print_r($_SERVER);
            //$postMax = ini_get('post_max_size'); //grab the size limits...
            $upload_max_size = ini_get('upload_max_filesiqze');
            //echo "upload max size is $upload_max_size <BR>";
            echo "<p style=\"color: #F00;\">\nPlease make sure files are 
            not larger than {$upload_max_size} <br></p>";
            // echo out error and solutions...
        }
        return true;
    }

    /**
     * process the form
     *
     * @param Array $values the array of values from the form
     * 
     * @return NULL
     */

    function _process($values)
    {
        print_r($values);
        if ($this->_saveFile($values)) {
            ///////////////////////////////////////////////////////////////////
            //////////////get the location of the last uploaded folder//////////
            //////////////////////////////////////////////////////////////////
            $db=& Database::singleton();
            $source_location = $db->pselectOne(
                "SELECT  SourceLocation FROM mri_upload
                WHERE DicomInserted = 0 ORDER BY UploadID DESC"
            );
            /////////////////////////////////////////////////////////////
            /////////get the path where the mri code is stored///////////////
            /////////////////////////////////////////////////////////////
            $config =& NDB_Config::singleton();
            $paths = $config->getSetting('paths');
            $mri_code_path = $paths['MRICodePath'];
            $mri_incoming_path = $paths['IncomingPath'];
            $mri_header_executer = $mri_code_path . "customize_headers.pl";
            $environment_script = $mri_code_path . "environment";
            $mri_log_file = $paths['data'] . "logs/mri_upload_log.log";
            $DICOMTAR = $mri_code_path. "dicom-archive/dicomTar.pl";
            $update_header = $mri_code_path. "dicom-archive/updateHeaders.pl";
            $tarchive_location =  $paths['data'] . "tarchive";
            $patient_name="";
            
            ///truncate the file if the log file exists:
            $today= getdate();
            $date = sprintf(
                "%04d-%02d-%02d", $today['year'], 
                $today['mon'], $today['mday']
            );
            $log_file = "/data/gusto/data/logs/" . "MRI_upload." .  $date . ".log";
            $f = @fopen($log_file, "r+");
            if ($f !== false) {
                ftruncate($f, 0);
                fclose($f);
            }
            ///////////////////////////////////////////////////////////////////////
            ////////////////////////1) untar or unzip the uploaded folder//////////
            ///////////////////////////////////////////////////////////////////////
            // decompress from gz
            //if it's a tar.gz or tgz////
            if ((preg_match("/.tgz/i", $source_location))
                || (preg_match("/.tar.gz/i", $source_location))
            ) {
                try {
                    $p = new PharData($source_location);
                    $p->decompress(); // creates /path/to/my.tar
                    $p->extractTo($mri_incoming_path);
                    //could remove the folder later once completed...
                } catch (Exception $e) {
                    print $e->getMessage();
                    // handle errors
                    print "error";
                }
                $this->log->addLog(
                    "The File $source_location is now
                    untarred into $mri_incoming_path \n <BR>"
                );
            }

            ///////////////////////////////////////////////////////////////////
            ///////////////////////////2) source the environment variable//////
            ///////////////////////////////////////////////////////////////////
            print "-----------------------------";
            $path = $mri_code_path. getenv('PATH');
            putenv("PATH=$path");
            $path = $mri_code_path. "uploadNeuroDB:". getenv('PERL5LIB');
            putenv("PERL5LIB=$path");
            putenv("TMPDIR=/tmp");
            putenv("HOME=/home/zia");
            putenv("USER=ZIA");
            $this->log->addLog("The Environment Variables are now set \n");

            ///////////////////////////////////////////////////////////////////
            //////////////////////run dicomtar and updateheaders for the///////
            /////////////////DICOM file////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////
            /*
            print "<BR>";
            $cmd = "source $environment_script 2>&1";
            print "command is $cmd";
            //exec(" /bin/bash $environment_script",$output);
            //echo shell_exec("cd $mri_code_path; . ./environment 2>&1");
            echo shell_exec("source $environment_script 2>&1");
            print "-----------------------------";
            print "<BR>";
            */
            ///////////////////////////////////////////////////////////////////////
            ///////////////////initialization//////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////
            $candid = $values['CandID'];
            $pscid = $values['PSCID'];
            $visit_label = $values['VisitLabel'];
            $file_name = pathinfo($source_location, PATHINFO_FILENAME);
            $file_name_without_exention = preg_replace(
                "/\\.[^.\\s]{3,4}$/", "", $source_location
            );
            print "file-name : $file_name, file-name-wo-extension: 
            $file_name_without_exention and sourcelocation $source_location";
            print "visitlabel $visit_label pscid: $pscid and candid : $candid";
            ///////////////////////////////////////////////////////////////////////
            /////////////////2) Running the Dicomtar and Update Headers////////////
            /////////////////// are set//////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////
            /////////////////if candid, pscid and visitabel text elements///////////
            /////////////////// are set//////////////////////////////////////
            //////////////////////////////////////////////////////////////////////
            if ((!empty($candid)) && (!empty($pscid)) &&(!empty($visit_label))) {
                //$query = "/usr/bin/perl  $mri_header_executer 
                //$file_name_without_exention &> $mri_log_file ";
                $patient_name = $pscid . "_" . $candid . "_" . $visit_label;
                $patient_id = "GUSTO";

                $cmd = "/usr/bin/perl $DICOMTAR $file_name_without_exention 
                $tarchive_location -clobber -database -profile prod ";
                print "cmd : $cmd \n";
                //$output = shell_exec($cmd);
                print $output;

                $row = $db->pselectRow(
                    "Select ArchiveLocation,TarchiveID FROM tarchive 
                    ORDER BY Lastupdate DESC"
                );
                $ArchiveLocation = $row['ArchiveLocation'];
                $tarchive_id = $row['TarchiveID'];
                $this->log->addLog(
                    "The DICOM file is now tarred into $ArchiveLocation \n <BR>"
                );

                if (isset($ArchiveLocation)) {
                    $cmd = "/usr/bin/perl$update_header $ArchiveLocation 
                    -verbose -set '(0010,0010)' $patient_name -set '(0010,0020)'
                     $patient_id -database  -profile prod";
                    print "cmd : $cmd \n";
                    print "query to be ran is $cmd";
                    print "-----------------------------";
                    print "<BR>";
                    //echo passthru($query);
                    //$output = shell_exec($cmd);
                    print $output;
                    print "-----------------------------";
                    print "<BR>";
                    $this->log->addLog(
                        "The DICOM tar file $ArchiveLocation is now updated 
                        with the following patientname: $patient_name patientid: 
                        $patient_id \n <BR> "
                    );
                    //exec("/usr/bin/perl  $mri_header_executer 
                    //$file_name_without_exention",$output);
                    //print_r($output);
                }
            } else {
                /////////////////////////////////////////////////////////////////
                /////////////////otherwise use the file-name////////////////////
                /////////////////////////////////////////////////////////////////
                $cmd = "/usr/bin/perl  $mri_header_executer 
                $file_name_without_exention &>> $mri_log_file ";
                $query = "/usr/bin/perl  $mri_header_executer 
                $file_name_without_exention ";
                print "query to be ran is $cmd";
                print "-----------------------------";
                print "<BR>";
                //echo passthru($query);
                //$output = shell_exec($cmd);
                print $output;
                print "-----------------------------";
                print "<BR>";
                $family_id = $this->getFamilyIDUsingFileName($file_name);
                $row=$db->pselectRow(
                    "Select ArchiveLocation,TarchiveID,PatientName FROM tarchive \
                    WHERE patientname LIKE concat ('%', :pname, '%') ",
                    array('pname'=>$family_id)
                );
                $tarchive_id = $row['TarchiveID'];
                $ArchiveLocation = $row['ArchiveLocation'];
                $patient_name = $row['PatientName'];
            }

            //////////////////////////////////////////////////////////////////////
            ////////////////////////write the output into a log file//////////////
            //////////////////////////////////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////
            ///////////////////////////////set the file-name to patient-name //////
            ///////////////////////////////////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////
            $new_file_location = $tarchive_location ."/". $patient_name . ".tar";
            $cmd = "mv $ArchiveLocation $new_file_location";
            print "command is $cmd";
            $output = shell_exec($cmd);
            print $output;
            ////////////////////////////////////////////////////////////////////////
            ////////////Insert the TarchiveID into the mri_upload table/////////////
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            if ((!(preg_match("/not processed/", $output))) 
                && ($tarchive_id!=null)
            ) {
                $db->update(
                    "mri_upload", array('TarchiveID'=>$tarchive_id, 
                    'DicomInserted'=>1),
                    array('SourceLocation'=>$source_location)
                );
                $this->tpl_data['dicom_success'] = true;
            }
            $this->tpl_data['upload_success'] = true;
        }
    }

    // methods available to all children
    /**
       * Save the file to the DataBase
       *
       * @param array $values the array of values from the form
       * 
       * @return true on success, false othewise
       * @access private
       */
    function _saveFile($values)
    {

        include_once "File_Upload.class.inc";

        //get the stored-file path
        $config =& NDB_Config::singleton();
        $paths = $config->getSetting('paths');
        $stored_file_path = $paths['IncomingPath'];

        //print $stored_file_path;
        $file=new File_Upload;

        $file->overwriteMode="reject";
        //pass the existing form ($form) by reference to File_Upload,
        // and register the file_upload field names being used.
        $file->registerForm($this->form);

        //Tell File_Upload what file handlers to use.
        $file->setFileHandler("mri_file", &$this);

        //Set the target directory that you want files moved 
        //into once they are validated and processed.
        $file->setBaseUploadDirectory($stored_file_path);
        $user =& User::singleton();
        if (PEAR::isError($user)) {
            return PEAR::raiseError(
                "User Error: ". $user->getMessage()
            );
        }

        //set the the IDs to the handler functions.
        $file->setHandlerArgs(array("values"=>$values));

        //proccesses them (including verify, move, and import steps)
        $file->processFiles();
        //echo error messages
        if (!empty($file->errorLog)) {
            while (list($fileType,$fileErrors)=each($file->errorLog)) {
                foreach ($fileErrors AS $error) {
                    echo "<span style='color:red'><b>Upload Error</b> 
                    $fileType: $error</span><br>";
                }
            }
            return false;
        }
        return true;
    }
    /**
     * Validate the file to be uploaded
     *
     * @param FILE  $file The uploaded file
     * @param Array $args The list of arguments
     * 
     * @return Boolean
     */
    function isValid($file, $args)
    {

        print_r($args);
        //make sure the file-name is  valid
        /**
         * 1) The patient-name and the vist-label do exist...
         * 2) if not correct the user can input the correct name...
         * 3) Make sure it's not corrupted..
         * 
         * 1) Ask them to use the database_based visit_labels:
         * 6month instead of 6mth
         * 6week or 6week_mri instead of 6wk
         * MRIday instead of Neonatal
         * 
         * 2) Also have the file-name follow the 
         * XXXXXXXXX_XXXX_XXXXXXX  (format)
         * not
         * XXXXXXXXX-XXXX_XXXXXXX
         * 
         * KKH100111-04029_Neonatal
         * 
         * 
         */
        $db=& Database::singleton();
        $errors = array();
        $config=&NDB_Config::singleton();
        $dicom_archive_settings = $config->getSetting('dicom_archive');
        $corect_name = $dicom_archive_settings['patientNameRegex'];
        $file_name = $file->fileInfo['name'];

        //////////////////////////////////////////////////////
        //////////////Make sure the extension is valid////////
        //////////////////////////////////////////////////////
        if (!(preg_match("/.zip/i", $file->fileInfo['name'])) 
            && (!(preg_match("/.tgz/i", $file->fileInfo['name']))) 
            && (!(preg_match("/.tar.gz/i", $file->fileInfo['name'])))
        ) {
            $errors[]="Incorrect File-name extention, The file-name 
            should either of type '.gz', '.zip' or 'tgz' ";
        }

        ////////////////////////////////////////////////////////////
        //////////////if the Advanced Options are filled out////////
        //////////////////////////////////////////////////////////
        $candid = $args['values']['CandID'];
        $pscid = $args['values']['PSCID'];
        $visit_label = $args['values']['VisitLabel'];
        print "visitlabel $visit_label pscid: $pscid and candid : $candid";
        if ((!empty($candid)) && (!empty($pscid)) &&(!empty($visit_label))) {
            if ($db->pselectOne(
                "SELECT COUNT(*) FROM candidate 
                WHERE candid = :cid", array('cid'=>$candid)
            )==0) {
                $errors[] = "The CandID: $candid Doesn't exist";
            }
            if ($db->pselectOne(
                "SELECT COUNT(*) FROM candidate WHERE PSCID = :pid",
                array('pid'=>$pscid)
            )==0) {
                $errors[] = "The PSCID: $pscid Doesn't exist";
            }

            if ($db->pselectOne(
                "SELECT COUNT(*) FROM session WHERE Visit_label = :vlabel",
                array('vlabel'=>$visit_label)
            )==0) {
                $errors[] = "The VisitLabel: $visit_label Doesn't exist";
            }
        } else {
            ////////////////////////////////////////////////////////////
            /////////Make sure the file-name follows the convention/////
            //////////i.e  KKH100111-04029_Neonatal/////////////////////
            ////////////////////////////////////////////////////////////
            $regex = "[a-zA-Z]{3}[0-9]{6}-[0-9]{5}_[a-zA-Z]*";
            $withoutExt = preg_replace("/\\.[^.\\s]{3,4}$/", "", $file_name);

            print "file without extension $withoutExt";
            //KKH100111-04029_Neonatal
            if (!(preg_match("/$regex/", $file_name))) {
                $errors[]="Incorrect File-name, The Correct Folder Name should
                be KKHdddddd_FamilyID_VisitLabel or NUHdddddd_FamilyID_VisitLabel";
            }

            ///////////////////////////////////////////////////////////////
            /////////////////Make sure the Visit_label exists///////////////
            ///////////////////////////////////////////////////////////////
            print("without extension is $withoutExt");
            list ($first, $visit_label)= split("_", $withoutExt);
            $visit_label = $this->getProperVisitLabel($visit_label);
            if ((!$this->VisitExists($visit_label)) || empty($visit_label)) {
                $errors[] = "The $visit_label in the folder-name is not valid";
            }

            ////////////////////////////////////////////////////////////
            ////////////////////////////Make sure the PSCID exist///////
            //////////////////////////////////////////////////////////////
            $family_id = $this->getFamilyIDUsingFileName($withoutExt);
            print "family id is $family_id and visitlabel is $visit_label <BR>";


            $pscid = $this->getPSCID($family_id);
            if (is_null($pscid) || empty($pscid)) {
                $errors[]=" The family id : $family_id in the folder-name is
                not valid, since it has no corresponding PSCID";
            }

            /////////////////////////////////////////////////////////////////////
            //////////////////Check to see if the dicom file is already////////// 
            //////////////////////////////inserted in the tarchive table/////////
            //////////////////////////////////////////////////////////////////////

            if (($db->pselectOne(
                "SELECT COUNT(*) FROM tarchive WHERE patientname 
                LIKE concat ('%', :pname, '%') ",
                array('pname'=>$family_id)
            ))>0) {
                $errors[]=" The Dicom file $file_name has already been inserted into 
                the tarchive table";
            }
            /////////////////////////////////////////////////////////////////////////
            ////////Check to see if the file has already been uploaded or not////////
            /////////////////////////////////////////////////////////////////////////

            if (($db->pselectOne(
                "SELECT COUNT(*) FROM mri_upload WHERE SourceLocation
                LIKE concat ('%', :fname, '%') ",
                array('fname'=>$file_name)
            ))>0) {
                $errors[]=" The file-name $file_name has already been uploaded";
            }
        }
        if (count($errors) > 0) {
            return $errors;    
        }
        
        return true;
    }

    /**
     * Import File-related fields into the 
     * Database
     *
     * @param FILE  $file The uploaded file
     * @param Array $args The list of arguments
     * 
     * @return boolean
     */
    function importFile($file, $args) 
    {
        $userid = $args['user_id'];
        $feedbacks = array();
        $db=& Database::singleton();

        //////////////////////////////////////
        ////////// create user object//////////
        ///////////////////////////////////////
        ////////////////////////////////////////
        //if empty get it using User class//////
        ////////////////////////////////////////
        $user_name = $args['user_id'];
        if (($user_name==null) || (!(isset($user_name)))) {
            $user =& User::singleton();
            if (PEAR::isError($user)) {
                return PEAR::raiseError("User Error: ".$user->getMessage());
            }
            $user_name = $user->getUsername();
        }

        /////////////////////////////////////////////////////
        ////////////////////Insert values into mri_upload/////
        /////////////////////////////////////////////////////
        //////////////////////////////////////
        //get the stored-file path/////////
        $config =& NDB_Config::singleton();
        $paths = $config->getSetting('paths');
        $stored_file_path = $paths['IncomingPath'];
        $file_name = $stored_file_path . $file->fileInfo['name'];
        $date = date('Y-m-d H:i:s');
        //only insert if the file-name doesn't exist in the table:
        //////////////////////////////////////////////////////////////
        /////Get the pscid, candid and visit_label
        ////////////////////////////////////////////////////////
        if ((!empty($args['values']['CandID'])) 
            && (!empty($args['values']['PSCID'])) 
            &&(!empty($args['values']['VisitLabel']))
        ) {
            ///////////////////////////////////////////////
            ////////////////////Use the arguments//////////////
            //////////////////////////////////////////////////
            $candid = $args['values']['CandID'];
            $pscid = $args['values']['PSCID'];
            $visit_label = $args['values']['VisitLabel'];
            print "visitlabel $visit_label pscid: $pscid and candid : $candid";
        } else {
            /////////////////////////////////////////////////////
            /////////////////Use the File-name///////////////////////
            /////////////////////////////////////////////////////
            $family_id = $this->getFamilyIDUsingFileName($file->fileInfo['name']);
            $pscid = $this->getPSCID($family_id);
            $candid = Utility::convert_ssid_to_dcc($pscid);
            list ($first, $visit_label)= split("_", $file->fileInfo['name']);
            print "visit_label is $visit_label";
            $visit_label = $this->getProperVisitLabel($visit_label);
        }
        $sessionid = $db->pselectOne(
            "SELECT ID FROM  session WHERE CandID = :cid AND Visit_label =:vlabel ",
            array('cid'=>$candid,'vlabel'=>$visit_label)
        );
        $values = array('FileType'=>'mnc','UploadedBy'=>$user_name,
        'UploadDate'=>$date,'UploadSuccess'=>1,'SourceLocation'=>$file_name,
        'SessionID'=>$sessionid);
        $db->Insert('mri_upload', $values);
        return true;
    }
    /**
     * Set the front-end DataTable
     *
     * @param INT $count Number of existing fields
     * 
     * @return boolean
     */
    function _setDataTableRows($count)
    {
        $x = 0;
        foreach ($this->list as $item) {
            $this->tpl_data['items'][$x][0]['value'] = $x + $count;
            $i = 1;
            foreach ($item as $key => $val) {
                $this->tpl_data['items'][$x][$i]['name'] = $key;
                $this->tpl_data['items'][$x][$i]['value'] = $val;

                $i++;
            }
            $x++;
        }
        return true;
    }

    /**
     * Mapps the file-name visit_label to the 
     * actual visit_label in the database
     *
     * @param STRING $visit THe visit_label
     * 
     * @return STRING
     */
    function getProperVisitLabel($visit)
    {
        $visit_label = $visit;
        if (preg_match("/6wk/", $visit)) {
            $visit_label = '6week_mri';
        }

        if (preg_match("/6mth/", $visit)) {
            $visit_label = '6month';
        }

        if (preg_match("/6wk/", $visit)) {
            $visit_label = '6week_mri';
        }

        if (preg_match("/Neonatal/", $visit)) {
            $visit_label = 'MRIday';
        }
        
        return $visit_label;
    }

    /**
     * Check to see if the given visit exists 
     * in the database
     *
     * @param STRING $visit The visit_label
     * 
     * @return boolean
     */
    function visitExists($visit)
    {
        $db=& Database::singleton();
        if (($db->pselectOne(
            "SELECT COUNT(*) FROM session WHERE Visit_label = :vlabel",
            array('vlabel'=>$visit)
        ))==0) {
            return false;
        }
        return true;
    }

    /**
     * Gets the PSCID using the family_id
     * Family_id is a substring of the PSCID
     *
     * @param STRING $family_id the family_id
     * 
     * @return STRING
     */
    function getPSCID($family_id)
    {
        $db=& Database::singleton();
        $pscid = $db->pselectOne(
            "SELECT PSCID FROM candidate WHERE PSCID like concat ('%', :fid, '%')",
            array('fid'=>$family_id)
        );
        return $pscid;
    }

    /**
     * Gets the CandID using the family_id
     * Family_id is a substring of the PSCID
     *
     * @param STRING $family_id the family_id
     * 
     * @return STRING
     */

    function getCandID($family_id)
    {
        $db=& Database::singleton();
        $candid = $db->pselectOne(
            "SELECT CandID FROM candidate WHERE PSCID like concat ('%', :fid, '%')",
            array('fid'=>$family_id)
        );
        return $candid;
    }

    
    /**
    * parses the file-name and extract the family-id
    *
    * @param STRING $fname the file-name
    * 
    * @return STRING
    */
    function getFamilyIDUsingFileName($fname)
    {
        list ($first, $visit_label)= split("_", $fname);
        list ($first, $family_id)= split("-", $first);
        return $family_id;
    }


    /**
    * getTargetDirectory retrieves info about where the file should be stored.
    *
    * The returned directory is ADDED to the baseUploadDirectory property.  
    * It should have a trailing slash.
    *
    * @param string $file The full filename including directory.
    * @param Array  $args The list of arguments
    * 
    * @return bool  $success if operation succeeded
    * @access public
    */
    function getTargetDirectory($file, $args)
    {
        $output="{$args["user_id"]}/";
        return $output;
    }


}
?>