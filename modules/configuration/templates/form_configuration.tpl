<script type="text/javascript" src="js/jquery/jquery.treegrid.js"></script>
<script type="text/javascript" src="js/jquery/jquery.treegrid.bootstrap3.js"></script>

<link rel="stylesheet" href="css/jquery.treegrid.css">

<p>Configuration Module Description</p>

{function name=printConfig}
    {foreach $items as $item}
        {if isset($item['Parent'])}
            <tr class="treegrid-{$item['ID']} treegrid-parent-{$item['Parent']}">
        {else}
            <tr class="treegrid-{$item['ID']}">
        {/if}
                <td>
                        {$item['Description']}
                </td>
                <td>
                    <div class="form-section" id="{$item['ID']}-formsection">
                        {if $item['AllowMultiple'] == 1}
                            <form method="POST" action="">
                                <button class="btn btn-default btn-sm add" id="{$item['ID']}" type="button" name="add-{$item['ID']}">
                                    <span class="glyphicon glyphicon-plus"></span> Add field
                                </button>
                            </form>
                        {/if}
                        {if isset($item['Value'])}
                            {foreach from=$item['Value'] key=k item=v}
                                <div class="form-item">
                                    <form method="POST">
                                        <input class="form-control input-sm" name="{$k}" type="text" id="{$k}" value="{$v}">
                                    </form>
                                {if $item['AllowMultiple'] == 1}
                                    <form method="POST">
                                        <button class="btn btn-default btn-small rm-btn" id="{$k}" type="submit" name="remove-{$k}">Remove</button>
                                    </form>
                                {/if}
                                </div>
                            {/foreach}
                        {/if}
                    </div>
                </td>
            </tr>
    {/foreach}
{/function}

<table class="tree table table-hover">
    {call name=printConfig items=$config}
</table>

