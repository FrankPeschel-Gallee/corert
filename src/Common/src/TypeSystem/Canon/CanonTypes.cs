﻿// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;

using Internal.NativeFormat;

using Debug = System.Diagnostics.Debug;

namespace Internal.TypeSystem
{
    /// <summary>
    /// Type of canonicalization applied to a type
    /// </summary>
    public enum CanonicalFormKind
    {
        /// <summary>
        /// Optimized for a particular set of instantiations (such as reference types)
        /// </summary>
        Specific,

        /// <summary>
        /// Canonicalization that works for any type
        /// </summary>
        Universal,

        /// <summary>
        /// Value used for lookup for Specific or Universal. Must not be used for canonicalizing.
        /// </summary>
        Any,
    }

    /// <summary>
    /// Base class for specialized and universal canon types
    /// </summary>
    public abstract partial class CanonBaseType : DefType
    {
        private TypeSystemContext _context;

        public CanonBaseType(TypeSystemContext context)
        {
            _context = context;
        }

        public sealed override TypeSystemContext Context
        {
            get
            {
                return _context;
            }
        }
    }

    /// <summary>
    /// Type used for specific canonicalization (e.g. for reference types)
    /// </summary>
    internal sealed partial class CanonType : CanonBaseType
    {
        public const string FullName = "System.__Canon";

        private int _hashcode;

        public CanonType(TypeSystemContext context)
            : base(context)
        {
            Initialize();
        }

        // Provides an extensibility hook for type system consumers that need to perform
        // consumer-specific initialization.
        partial void Initialize();

        public override DefType BaseType
        {
            get
            {
                return Context.GetWellKnownType(WellKnownType.Object);
            }
        }

        public override bool IsCanonicalSubtype(CanonicalFormKind policy)
        {
            return policy == CanonicalFormKind.Specific ||
                policy == CanonicalFormKind.Any;
        }

        protected override TypeDesc ConvertToCanonFormImpl(CanonicalFormKind kind)
        {
            Debug.Assert(kind == CanonicalFormKind.Specific);
            return this;
        }
        
        protected override TypeFlags ComputeTypeFlags(TypeFlags mask)
        {
            TypeFlags flags = 0;

            if ((mask & TypeFlags.CategoryMask) != 0)
            {
                flags |= TypeFlags.Class;
            }

            Debug.Assert((flags & mask) != 0);
            return flags;
        }

        public override int GetHashCode()
        {
            if (_hashcode == 0)
            {
                _hashcode = TypeHashingAlgorithms.ComputeNameHashCode(FullName);
            }

            return _hashcode;
        }

        public override string ToString()
        {
            return FullName;
        }
    }

    /// <summary>
    /// Type that can be used for canonicalization of any type (including value types of unknown size)
    /// </summary>
    internal sealed partial class UniversalCanonType : CanonBaseType
    {
        public const string FullName = "System.__UniversalCanon";

        private int _hashcode;

        public UniversalCanonType(TypeSystemContext context)
            : base(context)
        {
            Initialize();
        }

        // Provides an extensibility hook for type system consumers that need to perform
        // consumer-specific initialization.
        partial void Initialize();

        public override DefType BaseType
        {
            get
            {
                // Behavior for this is undefined.
                throw new NotSupportedException();
            }
        }

        public override bool IsCanonicalSubtype(CanonicalFormKind policy)
        {
            return policy == CanonicalFormKind.Universal || 
                policy == CanonicalFormKind.Any;
        }

        protected override TypeDesc ConvertToCanonFormImpl(CanonicalFormKind kind)
        {
            Debug.Assert(kind == CanonicalFormKind.Universal);
            return this;
        }

        protected override TypeFlags ComputeTypeFlags(TypeFlags mask)
        {
            TypeFlags flags = 0;

            if ((mask & TypeFlags.CategoryMask) != 0)
            {
                // Behavior for this is undefined.
                throw new NotSupportedException();
            }

            Debug.Assert((flags & mask) != 0);
            return flags;
        }

        public override int GetHashCode()
        {
            if (_hashcode == 0)
            {
                _hashcode = TypeHashingAlgorithms.ComputeNameHashCode(FullName);
            }

            return _hashcode;
        }

        public override string ToString()
        {
            return FullName;
        }
    }
}