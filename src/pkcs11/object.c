#include <stdio.h>

#include "sc-pkcs11.h"

#define min(a,b) (((a)<(b))?(a):(b))

static void dump_template(char *info, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount)
{
	int i, j;

	for (i = 0; i < ulCount; i++) {
		char foo[1024] = "";
		unsigned char *value = (unsigned char*) pTemplate[i].pValue;

		if (pTemplate[i].pValue) {
			if (pTemplate[i].ulValueLen < 16) {
				for (j = 0; j < pTemplate[i].ulValueLen; j++)
					sprintf(&foo[j*2], "%02X", value[j]);

				LOG("%s: Attribute 0x%x = %s (length=%d)\n",
				    info, pTemplate[i].type, foo, pTemplate[i].ulValueLen);
			} else {
				LOG("%s: Attribute 0x%x = ... (length=%d)\n",
				    info, pTemplate[i].type, pTemplate[i].ulValueLen);
			}
		} else {
			LOG("%s: Attribute 0x%x, length inquiry\n",
			    info, pTemplate[i].type);
		}
	}
}

CK_RV C_CreateObject(CK_SESSION_HANDLE hSession,    /* the session's handle */
		     CK_ATTRIBUTE_PTR  pTemplate,   /* the object's template */
		     CK_ULONG          ulCount,     /* attributes in template */
		     CK_OBJECT_HANDLE_PTR phObject) /* receives new object's handle. */
{
        LOG("C_CreateObject(%d, 0x%x, %d, 0x%d)\n", hSession, pTemplate, ulCount, phObject);
        return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_CopyObject(CK_SESSION_HANDLE    hSession,    /* the session's handle */
		   CK_OBJECT_HANDLE     hObject,     /* the object's handle */
		   CK_ATTRIBUTE_PTR     pTemplate,   /* template for new object */
		   CK_ULONG             ulCount,     /* attributes in template */
		   CK_OBJECT_HANDLE_PTR phNewObject) /* receives handle of copy */
{
	LOG("C_CopyObject(%d, %d, 0x%d, %d, 0x%x)\n",
	    hSession, hObject, pTemplate, ulCount, phNewObject);
        return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_DestroyObject(CK_SESSION_HANDLE hSession,  /* the session's handle */
		      CK_OBJECT_HANDLE  hObject)   /* the object's handle */
{
        LOG("C_DestroyObject(%d, %d)\n", hSession, hObject);
        return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_GetObjectSize(CK_SESSION_HANDLE hSession,  /* the session's handle */
		      CK_OBJECT_HANDLE  hObject,   /* the object's handle */
		      CK_ULONG_PTR      pulSize)   /* receives size of object */
{
        LOG("C_GetObjectSize(%d, %d, 0x%x)\n", hSession, hObject, pulSize);
        return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_GetAttributeValue(CK_SESSION_HANDLE hSession,   /* the session's handle */
			  CK_OBJECT_HANDLE  hObject,    /* the object's handle */
			  CK_ATTRIBUTE_PTR  pTemplate,  /* specifies attributes, gets values */
			  CK_ULONG          ulCount)    /* attributes in template */
{
	struct pkcs11_slot *slt;
	struct pkcs11_object *object;
        int i, j;

        LOG("C_GetAttributeValue(%d, %d, 0x%x, %d)\n", hSession, hObject, pTemplate, ulCount);
	dump_template("C_GetAttributeValue", pTemplate, ulCount);

	if (hSession < 1 || hSession > PKCS11_MAX_SESSIONS || session[hSession] == NULL)
		return CKR_SESSION_HANDLE_INVALID;
	slt = &slot[session[hSession]->slot];

	if (hObject < 1 || hObject > slt->num_objects)
                return CKR_OBJECT_HANDLE_INVALID;

	object = slt->object[hObject];

	LOG("C_GetAttributeValue: Slot %d, Object: 0x%x,  Attributes: %d\n",
	    session[hSession]->slot, object, object->num_attributes);

	for (i = 0; i < ulCount; i++) {
		// For each request attribute

                // 1. Find matching attribute
		for (j = 0; j < object->num_attributes; j++) {
			if (pTemplate[i].type == object->attribute[j].type)
                                break;
		}

		// 2. If object doesn't posses attribute
		if (j >= object->num_attributes) {
			pTemplate[i].ulValueLen = -1;
                        continue;
		}

		// 3. If pValue is NULL_PTR then it's a size inquiry
		if (pTemplate[i].pValue == NULL_PTR) {
			pTemplate[i].ulValueLen = object->attribute[j].ulValueLen;

			LOG("C_GetAttributeValue: Attribute 0x%x length %d\n",
			    pTemplate[i].type, object->attribute[j].ulValueLen);
                        continue;
		}

		// 4. If value fits then copy it and update true length
		if (pTemplate[i].ulValueLen >= object->attribute[j].ulValueLen) {
			LOG("C_GetAttributeValue: Copying attribute 0x%x length %d\n",
			    pTemplate[i].type, object->attribute[j].ulValueLen);
			pTemplate[i].ulValueLen = object->attribute[j].ulValueLen;
			memcpy(pTemplate[i].pValue,
			       object->attribute[j].pValue,
			       object->attribute[j].ulValueLen);
                        continue;
		}

		// 5. Otherwise set length to minus one
		pTemplate[i].ulValueLen = -1;
	}

        return CKR_OK;
}

CK_RV C_SetAttributeValue(CK_SESSION_HANDLE hSession,   /* the session's handle */
			  CK_OBJECT_HANDLE  hObject,    /* the object's handle */
			  CK_ATTRIBUTE_PTR  pTemplate,  /* specifies attributes and values */
			  CK_ULONG          ulCount)    /* attributes in template */
{
        LOG("C_SetAttributeValue(%d, %d, 0x%x, %d)\n", hSession, hObject, pTemplate, ulCount);
	dump_template("C_SetAttributeValue", pTemplate, ulCount);

        return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_FindObjectsInit(CK_SESSION_HANDLE hSession,   /* the session's handle */
			CK_ATTRIBUTE_PTR  pTemplate,  /* attribute values to match */
			CK_ULONG          ulCount)    /* attributes in search template */
{
	struct pkcs11_session *ses;

	LOG("C_FindObjectsInit(%d, %d, 0x%x, %d)\n", hSession, pTemplate, ulCount);
	dump_template("C_FindObjectsInit", pTemplate, ulCount);

	if (hSession < 1 || hSession >= PKCS11_MAX_SESSIONS || session[hSession] == NULL)
		return CKR_SESSION_HANDLE_INVALID;

	if (ulCount != 1 || pTemplate[0].type != CKA_CLASS)
		return CKR_ATTRIBUTE_TYPE_INVALID;

	if (pTemplate[0].ulValueLen != 4 || *((CK_ULONG_PTR) pTemplate[0].pValue) != CKO_CERTIFICATE)
		return CKR_ATTRIBUTE_VALUE_INVALID;

        ses = session[hSession];
        ses->search.num_matches = 1;
        ses->search.position = 0;
        ses->search.handles[0] = 1;

        return CKR_OK;
}

CK_RV C_FindObjects(CK_SESSION_HANDLE    hSession,          /* the session's handle */
		    CK_OBJECT_HANDLE_PTR phObject,          /* receives object handle array */
		    CK_ULONG             ulMaxObjectCount,  /* max handles to be returned */
		    CK_ULONG_PTR         pulObjectCount)    /* actual number returned */
{
	struct pkcs11_session *ses;
        int to_return;

	LOG("C_FindObjects(%d, 0x%x, %d, 0x%x)\n", hSession, phObject, ulMaxObjectCount, pulObjectCount);
	if (hSession < 1 || hSession >= PKCS11_MAX_SESSIONS || session[hSession] == NULL)
		return CKR_SESSION_HANDLE_INVALID;

	ses = session[hSession];

	to_return = min(ulMaxObjectCount, ses->search.num_matches - ses->search.position);
	*pulObjectCount = to_return;

	memcpy(phObject,
	       &ses->search.handles[ses->search.position],
	       to_return * sizeof(CK_OBJECT_HANDLE));

        ses->search.position += to_return;

        return CKR_OK;
}

CK_RV C_FindObjectsFinal(CK_SESSION_HANDLE hSession) /* the session's handle */
{
	struct pkcs11_session *ses;

        LOG("C_FindObjectsFinal(%d)\n", hSession);
	if (hSession < 1 || hSession >= PKCS11_MAX_SESSIONS || session[hSession] == NULL)
		return CKR_SESSION_HANDLE_INVALID;

	ses = session[hSession];
        ses->search.num_matches = 0;
        ses->search.position = 0;

        return CKR_OK;
}


