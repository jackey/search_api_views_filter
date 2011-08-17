<?php
/**
 * Implementation of hook_views_handlers().
 */
  function search_api_views_filter_views_handlers() {
	return array(
		'info' => array('path' => drupal_get_path('module', 'search_api_views_filter') . '/views'),
		'handler' => array(
			'search_api_views_filter_in_operator' => array('parent' => 'views_handler_filter')
		),
	);
  }

  /**
   * Implementation of hook_views_data().
   */
  function search_api_views_filter_views_data_alter(&$data) {
    if ($data['node']['nid']) {
      foreach (search_api_index_load_multiple(FALSE) as $index) {
      	if ($index->enabled) {
          $data['node'][$index->machine_name] = array(
            'title' => t('Seach api results From ' . $index->name),
            'help' => t('Filter by search api results'),
            'real field' => 'nid',
            'filter' =>  array('handler' => 'search_api_views_filter_in_operator', 'search_api_index' => $index->machine_name),
          );
      	}
      }
    }
  }
?>