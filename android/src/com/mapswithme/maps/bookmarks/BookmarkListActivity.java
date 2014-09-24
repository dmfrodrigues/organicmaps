package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.EditText;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MapsWithMeBaseListActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.Utils;


public class BookmarkListActivity extends MapsWithMeBaseListActivity
{
  public static final String TAG = "BookmarkListActivity";

  private BookmarkManager mManager;
  private EditText mSetName;
  private BookmarkCategory mEditedSet;
  private int mSelectedPosition;
  private BookmarkListAdapter mPinAdapter;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.bookmarks_list);

    mManager = BookmarkManager.getBookmarkManager(getApplicationContext());

    // Initialize with passed edited set.
    final int setIndex = getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1);
    mEditedSet = mManager.getCategoryById(setIndex);

    setTitle(mEditedSet.getName());
    createListAdapter();

    setUpViews();

    getListView().setOnItemClickListener(new OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        switch (mPinAdapter.getItemViewType(position))
        {
        case BookmarkListAdapter.TYPE_SECTION:
          return;
        case BookmarkListAdapter.TYPE_BMK:
          final Bookmark bmk = (Bookmark) mPinAdapter.getItem(position);
          mManager.showBookmarkOnMap(setIndex, bmk.getBookmarkId());
          break;
        case BookmarkListAdapter.TYPE_TRACK:
          final Track track = (Track) mPinAdapter.getItem(position);
          Framework.nativeShowTrackRect(track.getCategoryId(), track.getTrackId());
          break;
        }

        final Intent i = new Intent(BookmarkListActivity.this, MWMActivity.class);
        i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(i);
      }
    });

    registerForContextMenu(getListView());
    adaptUiForOsVersion();
  }

  private void adaptUiForOsVersion()
  {
    final Button shareButton = (Button) findViewById(R.id.btn_share_bookmarks);
    if (Utils.apiLowerThan(11))
    {
      shareButton.setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v) { onSendEMail(); }
      });
    }
    else
      shareButton.setVisibility(View.GONE);
  }

  private void createListAdapter()
  {
    mPinAdapter = new BookmarkListAdapter(this, mEditedSet);

    setListAdapter(mPinAdapter);

    mPinAdapter.startLocationUpdate();
  }

  private void assignCategoryParams()
  {
    final String name = mSetName.getText().toString();
    if (!name.equals(mEditedSet.getName()))
      mManager.setCategoryName(mEditedSet, name);
  }

  private void setUpViews()
  {
    mSetName = (EditText) findViewById(R.id.pin_set_name);
    mSetName.setText(mEditedSet.getName());
    mSetName.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        setTitle(s.toString());

        // Note! Do not set actual name here - saving process may be too long
        // see assignCategoryParams() instead.
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after)
      {
      }

      @Override
      public void afterTextChanged(Editable s)
      {
      }
    });
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    assignCategoryParams();

    // Some list views can be section delimiters.
    if (menuInfo instanceof AdapterView.AdapterContextMenuInfo)
    {
      final AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;

      mSelectedPosition = info.position;
      final Object obj = mPinAdapter.getItem(mSelectedPosition);
      final int type = mPinAdapter.getItemViewType(mSelectedPosition);

      if (type == BookmarkListAdapter.TYPE_BMK)
      {
        final MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.pin_sets_context_menu, menu);

        for (final ShareAction sa : ShareAction.ACTIONS.values())
        {
          if (sa.isSupported(this))
            menu.add(Menu.NONE, sa.getId(), sa.getId(), getResources().getString(sa.getNameResId()));
        }

        menu.setHeaderTitle(((Bookmark) obj).getName());
      }
      else if (type == BookmarkListAdapter.TYPE_TRACK)
      {
        menu.add(Menu.NONE, MENU_DELETE_TRACK, MENU_DELETE_TRACK, getString(R.string.delete));
        menu.setHeaderTitle(((Track) obj).getName());
      }

      super.onCreateContextMenu(menu, v, menuInfo);
    }
  }

  private final static int MENU_DELETE_TRACK = 0x42;

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    final int itemId = item.getItemId();
    final Object obj = mPinAdapter.getItem(mSelectedPosition);

    if (itemId == R.id.set_edit)
    {
      startPinActivity(mEditedSet.getId(), ((Bookmark) obj).getBookmarkId());
    }
    else if (itemId == R.id.set_delete)
    {
      mManager.deleteBookmark((Bookmark) obj);
      mPinAdapter.notifyDataSetChanged();
    }
    else if (ShareAction.ACTIONS.containsKey(itemId))
    {
      final ShareAction shareAction = ShareAction.ACTIONS.get(itemId);
      shareAction.shareMapObject(this, (Bookmark) obj);
    }
    else if (itemId == MENU_DELETE_TRACK)
    {
      mManager.deleteTrack((Track) obj);
      mPinAdapter.notifyDataSetChanged();
    }

    return super.onContextItemSelected(item);
  }

  private void startPinActivity(int cat, int bmk)
  {
    startActivity(new Intent(this, BookmarkActivity.class)
        .putExtra(BookmarkActivity.PIN, new ParcelablePoint(cat, bmk)));
  }

  @Override
  protected void onStart()
  {
    super.onStart();

    mPinAdapter.notifyDataSetChanged();
  }

  @Override
  protected void onPause()
  {
    assignCategoryParams();

    mPinAdapter.stopLocationUpdate();

    super.onPause();
  }

  @Override
  protected void onResume()
  {
    super.onResume();

    mPinAdapter.startLocationUpdate();
  }

  private void onSendEMail()
  {
    assignCategoryParams();

    String path = ((MWMApplication) getApplication()).getTempPath();
    final String name = mManager.saveToKMZFile(mEditedSet.getId(), path);
    if (name == null)
    {
      // some error occurred
      return;
    }

    final Intent intent = new Intent(Intent.ACTION_SEND);
    intent.putExtra(android.content.Intent.EXTRA_SUBJECT, getString(R.string.share_bookmarks_email_subject));
    intent.putExtra(android.content.Intent.EXTRA_TEXT, String.format(getString(R.string.share_bookmarks_email_body), name));

    path = path + name + ".kmz";
    Log.d(TAG, "KMZ file path = " + path);
    intent.putExtra(android.content.Intent.EXTRA_STREAM, Uri.parse("file://" + path));
    intent.setType("application/vnd.google-earth.kmz");

    try
    {
      startActivity(Intent.createChooser(intent, getString(R.string.share_by_email)));
    } catch (final Exception ex)
    {
      Log.i(TAG, "Can't run E-Mail activity" + ex);
    }
  }

  // Menu routines
  private static final int ID_SEND_BY_EMAIL = 0x01;

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    final MenuItem menuItem = menu.add(Menu.NONE,
        ID_SEND_BY_EMAIL,
        ID_SEND_BY_EMAIL,
        R.string.share_by_email);

    menuItem.setIcon(R.drawable.share);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
      menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);

    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == ID_SEND_BY_EMAIL)
    {
      onSendEMail();
      return true;
    }

    return super.onOptionsItemSelected(item);
  }
}
